#include "Lesson5.h"
#include "D3D12LibPCH.h"
#include <ShObjIdl.h>  // For IFileOpenDialog
#include <shlwapi.h>
#include "helpers.h"
#include <regex>
#include <iostream>
#include <DirectXColors.h>
#include "MeshHelper.h"
using namespace DirectX;

XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR _position, FXMVECTOR _direction, FXMVECTOR  _up)
{
	assert(!XMVector3Equal(_direction, XMVectorZero()));
	assert(!XMVector3IsInfinite(_direction));
	assert(!XMVector3Equal(_up, XMVectorZero()));
	assert(!XMVector3IsInfinite(_up));

	XMVECTOR R2 = XMVector3Normalize(_direction);
	XMVECTOR R0 = XMVector3Cross(_up, R2);
	R0 = XMVector3Normalize(R0);

	XMVECTOR R1 = XMVector3Cross(R2, R0);

	XMMATRIX M(R0, R1, R2, _position);

	return M;
}

Lesson5::Lesson5(const std::wstring& _name, int _width, int _height, bool _vSync /*= false*/)
	:m_ScissorRect{0, 0, LONG_MAX, LONG_MAX},
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height))),
	m_AnimateLights(false),
	m_Fullscreen(false),
	m_AllowFullscreenToggle(true),
	m_Width(_width),
	m_Height(_height),
	m_ShowFileOpenDialog(false),
	m_CancelLoading(false),
	m_ShowControls(false),
	m_IsLoading(false),
	m_FPS(0.0f)
{
#if defined(_DEBUG)
	Device::EnableDebufLayer();
#endif

	XMVECTOR cameraPos = XMVectorSet(0, 5, -5, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_Camera.SetLookAt(cameraPos, cameraTarget, cameraUp);
	float aspect = m_Width / (float)m_Height;
	m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);

	m_Window = Application::Get().CreateWindow(_name, _width, _height);

	//注册回调函数
	m_Window->Update += UpdateEvent::slot(&Lesson5::OnUpdate, this);
	m_Window->Resize += ResizeEvent::slot(&Lesson5::OnResize, this);
	m_Window->DPIScaleChanged += DPIScaleEvent::slot(&Lesson5::OnDPIScaleChanged, this);
	m_Window->KeyPressed += KeyboardEvent::slot(&Lesson5::OnKeyPressed, this);
	m_Window->KeyReleased += KeyboardEvent::slot(&Lesson5::OnKeyReleased, this);
	m_Window->MouseMoved += MouseMotionEvent::slot(&Lesson5::OnMouseMoved, this);
}

Lesson5::~Lesson5()
{
	std::cout << "5析构" << std::endl;
}

uint32_t Lesson5::Run()
{
	LoadContent();

	m_Window->Show();

	auto retCode = Application::Get().Run();

	//确保加载完成
	//m_LoadingTask.get();

	UnLoadContent();

	return retCode;
}

void Lesson5::LoadContent()
{
	m_Device = Device::Create();
	m_SwapChain = m_Device->CreateSwapChain(m_Window->GetWindowHandle(), DXGI_FORMAT_R8G8B8A8_UNORM);

	// Start the loading task to perform async loading of the scene file.
	//LoadScene(L"C:\\Code\\DX12New\\Assets\\Models\\sphere.fbx");
	LoadScene(L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj");
	//m_LoadingTask = std::async(std::launch::async, std::bind(&Lesson5::LoadScene, this,L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj"));

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue.GetCommandList();

	m_Sphere = MeshHelper::CreateSphere(commandList, 0.1f);
	m_Cone = MeshHelper::CreateCone(commandList, 0.1f, 0.2f);
	m_Axis = commandList->LoadSceneFromFile(L"C:\\Code\\DX12New\\Assets\\Models\\axis_of_evil.nff");

	//创建PSO
	m_LightingPSO = std::make_shared<EffectPSO>(m_Device, true, false);
	m_DecalPSO = std::make_shared<EffectPSO>(m_Device, true, true);
	m_UnlitPSO = std::make_shared<EffectPSO>(m_Device, false, false);

	const int numDirectionalLights = 3;
	static const XMVECTORF32 LightColors[] = { DirectX::Colors::White, DirectX::Colors::OrangeRed, DirectX::Colors::Blue };

	static float lightAnimTime = 0.0f;

	const float radius = 1.0f;
	float       directionalLightOffset = numDirectionalLights > 0 ? 2.0f * XM_PI / numDirectionalLights : 0;

	//XMMATRIX viewMatrix = m_Camera.GetViewMatrix();
	m_DirectionalLights.resize(numDirectionalLights);
	for (int i = 0; i < numDirectionalLights; ++i)
	{
		DirectionalLight& l = m_DirectionalLights[i];

		float angle = lightAnimTime + directionalLightOffset * i;

		XMVECTORF32 positionWS = { static_cast<float>(std::cos(angle)) * radius,
								   static_cast<float>(std::sin(angle)) * radius, radius, 1.0f };

		//XMVECTOR directionWS = XMVector3Normalize(XMVectorNegate(positionWS));
		XMVECTOR directionWS = XMVectorSet(i + 0.1 * i, i + 0.1 * i, i + 0.1 * i, 0);
		//XMVECTOR directionVS = XMVector3TransformNormal(directionWS, viewMatrix);
		XMVECTOR directionVS = XMVectorSet(i + 0.1 * i, i + 0.1 * i, i + 0.1 * i, 0);

		XMStoreFloat4(&l.DirectionWS, directionWS);
		XMStoreFloat4(&l.DirectionVS, directionVS);

		l.Color = XMFLOAT4(LightColors[i]);
	}

	auto fence = commandQueue.ExecuteCommandList(commandList);

	// Create a color buffer with sRGB for gamma correction.
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	colorDesc.MipLevels = 1;
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	auto colorTexture = m_Device->CreateTexture(colorDesc, &colorClearValue);
	colorTexture->SetName(L"Color Render Target");

	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
		sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	auto depthTexture = m_Device->CreateTexture(depthDesc, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");

	// Attach the textures to the render target.
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	// Make sure the copy command queue is finished before leaving this function.
	commandQueue.WaitForFenceValue(fence);
}

void Lesson5::UnLoadContent()
{

}

void Lesson5::OnUpdate(UpdateEventArgs& e)
{
	static uint64_t frameCount = 0;
	static double   totalTime = 0.0;

	totalTime += e.DeltaTime;
	frameCount++;

	if (totalTime > 1.0)
	{
		m_FPS = frameCount / totalTime;

		wchar_t buffer[512];
		::swprintf_s(buffer, L"Models [FPS: %f]", m_FPS);
		m_Window->SetWindowTitle(buffer);

		frameCount = 0;
		totalTime = 0.0;
	}

	float speedMultipler = 20.0f;

	XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.DeltaTime);
	XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.DeltaTime);
	m_Camera.Translate(cameraTranslate, Space::Local);
	m_Camera.Translate(cameraPan, Space::Local);

	XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
	m_Camera.SetRotation(cameraRotation);

	XMMATRIX viewMatrix = m_Camera.GetViewMatrix();

	XMVECTOR cameraPoint = DirectX::XMVectorSet(0, 0, 0, 1);
	XMMATRIX translationMatrix = XMMatrixTranslationFromVector(cameraPoint);
	XMMATRIX scaleMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);
	m_Axis->GetRootNode()->SetLocalTransform(scaleMatrix * translationMatrix);



	m_LightingPSO->SetDirectionalLights(m_DirectionalLights);
	m_DecalPSO->SetDirectionalLights(m_DirectionalLights);

	m_SwapChain->WaitForSwapChain();

	Application::Get().ProcessInput();

	OnRender();
}

void Lesson5::OnResize(ResizeEventArgs& e)
{
	m_Width = std::max(1, e.Width);
	m_Height = std::max(1, e.Height);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	float aspect = m_Width / (float)m_Height;
	m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);

	m_RenderTarget.Resize(m_Width, m_Height);

	m_SwapChain->Resize(m_Width, m_Height);
}

void Lesson5::OnRender()
{
	m_Window->SetFullScreen(m_Fullscreen);

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto  commandList = commandQueue.GetCommandList();

	const auto& rendertarget = m_IsLoading ? m_SwapChain->GetRenderTarget() : m_RenderTarget;

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	if (m_IsLoading)
	{
		
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color0), clearColor);
	}
	else
	{
		SceneVisitor opaquePass(*commandList, m_Camera, *m_LightingPSO, false);
		SceneVisitor transparentPass(*commandList, m_Camera, *m_DecalPSO, true);
		SceneVisitor unlitPass(*commandList, m_Camera, *m_UnlitPSO, false);


		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(rendertarget.GetTexture(AttachmentPoint::DepthStencil),D3D12_CLEAR_FLAG_DEPTH);
		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(rendertarget);

		m_Scene->Accept(opaquePass);
		
		m_Axis->Accept(unlitPass);
		m_Scene->Accept(transparentPass);

		MaterialProperties lightMaterial = Material::Black;
		for (const auto& l : m_PointLights)
		{
			lightMaterial.Emissive = l.Color;
			auto lightPos = XMLoadFloat4(&l.PositionWS);
			auto worldMatrix = XMMatrixTranslationFromVector(lightPos);

			m_Sphere->GetRootNode()->SetLocalTransform(worldMatrix);
			m_Sphere->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(lightMaterial);
			m_Sphere->Accept(unlitPass);
		}

		for (const auto& l : m_SpotLights)
		{
			lightMaterial.Emissive = l.Color;
			XMVECTOR lightPos = XMLoadFloat4(&l.PositionWS);
			XMVECTOR lightDir = XMLoadFloat4(&l.DirectionWS);
			XMVECTOR up = XMVectorSet(0, 1, 0, 0);

			// Rotate the cone so it is facing the Z axis.
			auto rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90.0f));
			auto worldMatrix = rotationMatrix * LookAtMatrix(lightPos, lightDir, up);

			m_Cone->GetRootNode()->SetLocalTransform(worldMatrix);
			m_Cone->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(lightMaterial);
			m_Cone->Accept(unlitPass);
		}

		auto swapChainBackBuffer = m_SwapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
		auto msaaRenderTarget = m_RenderTarget.GetTexture(AttachmentPoint::Color0);

		commandList->ResolveSubresource(swapChainBackBuffer, msaaRenderTarget);
	}
	
	commandQueue.ExecuteCommandList(commandList);

	m_SwapChain->Present();
}

void Lesson5::OnKeyPressed(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::Escape:
		Application::Get().Stop();
		break;
	case KeyCode::Space:
		m_AnimateLights = !m_AnimateLights;
		break;
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		if (m_AllowFullscreenToggle)
		{
			m_Fullscreen = !m_Fullscreen;  // Defer window resizing until OnUpdate();
			// Prevent the key repeat to cause multiple resizes.
			m_AllowFullscreenToggle = false;
		}
		break;
		}
	case KeyCode::V:
		m_SwapChain->ToggleVSync();
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 1.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 1.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 1.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 1.0f;
		break;
	case KeyCode::Q:
		m_Down = 1.0f;
		break;
	case KeyCode::E:
		m_Up = 1.0f;
		break;
	case KeyCode::O:
		if (e.Control)
		{
			OpenFile();
		}
		break;
	}
}

void Lesson5::OnKeyReleased(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		m_AllowFullscreenToggle = true;
		}
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 0.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 0.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 0.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 0.0f;
		break;
	case KeyCode::Q:
		m_Down = 0.0f;
		break;
	case KeyCode::E:
		m_Up = 0.0f;
		break;
	case KeyCode::ShiftKey:
		m_Shift = false;
		break;
	}
}

void Lesson5::OnMouseMoved(MouseMotionEventArgs& e)
{
	const float mouseSpeed = 0.1f;
	if (e.LeftButton)
	{
		m_Pitch += e.RelY * mouseSpeed;

		m_Pitch = Clamp(m_Pitch, -90.0f, 90.0f);

		m_Yaw += e.RelX * mouseSpeed;
	}
}

void Lesson5::OnDPIScaleChanged(DPIScaleEventArgs& e)
{

}

void Lesson5::OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget)
{

}

bool Lesson5::LoadScene(const std::wstring& sceneFile)
{
	//用于表示std:：bind的占位符参数
	using namespace std::placeholders;

	m_IsLoading = true;
	m_CancelLoading = false;

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue.GetCommandList();

	m_LoadingText = std::string("Loading") + ConvertString(sceneFile) + "...";
	auto scene = commandList->LoadSceneFromFile(sceneFile, std::bind(&Lesson5::LoadingProgress, this, _1));

	if (scene)
	{
		//缩放场景,以适合摄像机时锥
		DirectX::BoundingSphere s;
		BoundingSphere::CreateFromBoundingBox(s, scene->GetAABB());
		auto scale = 50.0f / (s.Radius * 2.0f);
		s.Radius *= scale;

		scene->GetRootNode()->SetLocalTransform(XMMatrixScaling(scale, scale, scale));

		m_Scene = scene;
	}

	commandQueue.ExecuteCommandList(commandList);

	//确保场景已经加载完成
	commandQueue.Flush();

	m_IsLoading = false;

	return m_Scene != nullptr;
}

void Lesson5::OpenFile()
{
	// clang-format off
	static const COMDLG_FILTERSPEC g_FileFilters[] = {
		{ L"Autodesk", L"*.fbx" },
		{ L"Collada", L"*.dae" },
		{ L"glTF", L"*.gltf;*.glb" },
		{ L"Blender 3D", L"*.blend" },
		{ L"3ds Max 3DS", L"*.3ds" },
		{ L"3ds Max ASE", L"*.ase" },
		{ L"Wavefront Object", L"*.obj" },
		{ L"Industry Foundation Classes (IFC/Step)", L"*.ifc" },
		{ L"XGL", L"*.xgl;*.zgl" },
		{ L"Stanford Polygon Library", L"*.ply" },
		{ L"AutoCAD DXF", L"*.dxf" },
		{ L"LightWave", L"*.lws" },
		{ L"LightWave Scene", L"*.lws" },
		{ L"Modo", L"*.lxo" },
		{ L"Stereolithography", L"*.stl" },
		{ L"DirectX X", L"*.x" },
		{ L"AC3D", L"*.ac" },
		{ L"Milkshape 3D", L"*.ms3d" },
		{ L"TrueSpace", L"*.cob;*.scn" },
		{ L"Ogre XML", L"*.xml" },
		{ L"Irrlicht Mesh", L"*.irrmesh" },
		{ L"Irrlicht Scene", L"*.irr" },
		{ L"Quake I", L"*.mdl" },
		{ L"Quake II", L"*.md2" },
		{ L"Quake III", L"*.md3" },
		{ L"Quake III Map/BSP", L"*.pk3" },
		{ L"Return to Castle Wolfenstein", L"*.mdc" },
		{ L"Doom 3", L"*.md5*" },
		{ L"Valve Model", L"*.smd;*.vta" },
		{ L"Open Game Engine Exchange", L"*.ogx" },
		{ L"Unreal", L"*.3d" },
		{ L"BlitzBasic 3D", L"*.b3d" },
		{ L"Quick3D", L"*.q3d;*.q3s" },
		{ L"Neutral File Format", L"*.nff" },
		{ L"Sense8 WorldToolKit", L"*.nff" },
		{ L"Object File Format", L"*.off" },
		{ L"PovRAY Raw", L"*.raw" },
		{ L"Terragen Terrain", L"*.ter" },
		{ L"Izware Nendo", L"*.ndo" },
		{ L"All Files", L"*.*" }
	};
	// clang-format on

	ComPtr<IFileOpenDialog> pFileOpen;
	HRESULT                 hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOpen));

	if (SUCCEEDED(hr))
	{
		// Create an event handling object, and hook it up to the dialog.
		// ComPtr<IFileDialogEvents> pDialogEvents;
		// hr = DialogEventHandler_CreateInstance( IID_PPV_ARGS( &pDialogEvents ) );

		if (SUCCEEDED(hr))
		{
			// Setup filters.
			hr = pFileOpen->SetFileTypes(_countof(g_FileFilters), g_FileFilters);
			pFileOpen->SetFileTypeIndex(40);  // All Files (*.*)

			// Show the open dialog box.
			if (SUCCEEDED(pFileOpen->Show(m_Window->GetWindowHandle())))
			{
				ComPtr<IShellItem> pItem;
				if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						// try to load the scene file (asynchronously).
						m_LoadingTask =
							std::async(std::launch::async, std::bind(&Lesson5::LoadScene, this, pszFilePath));

						CoTaskMemFree(pszFilePath);
					}
				}
			}
		}
	}
}

bool Lesson5::LoadingProgress(float _loadingProgress)
{
	m_LoadingProgress = _loadingProgress;

	return !m_CancelLoading;
}
