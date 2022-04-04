#include "Lesson5.h"
#include "D3D12LibPCH.h"
#include <ShObjIdl.h>  // For IFileOpenDialog
#include <shlwapi.h>
#include "helpers.h"
#include <regex>
#include <iostream>
#include <DirectXColors.h>
#include "MeshHelper.h"
#include "ShaderDefinition.h"
using namespace DirectX;

Matrix4 XM_CALLCONV LookAtMatrix(Vector4 _position, Vector4 _direction, Vector4  _up)
{
	assert(!Transform::Vector3Equal(_direction, Vector4(XMVectorZero())));
	assert(!Transform::Vector3IsInfinite(_direction));
	assert(!Transform::Vector3Equal(_up, Vector4(XMVectorZero())));
	assert(!Transform::Vector3IsInfinite(_up));

	Vector4 R2 = Transform::Vector3Normalize(_direction);
	Vector4 R0 = Transform::Vector3Cross(_up, R2);
	R0 = Transform::Vector3Normalize(R0);

	Vector4 R1 = Transform::Vector3Cross(R2, R0);

	Matrix4 M(R0, R1, R2, _position);

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
	m_FPS(0.0f),
	m_CameraController(m_Camera)
{
	Vector4 a = Vector4(1.0, 1.0, 1.0, 1.0);
#if defined(_DEBUG)
	Device::EnableDebufLayer();
#endif
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
	m_SwapChain = m_Device->CreateSwapChain(m_Window->GetWindowHandle(), DXGI_FORMAT_R16G16B16A16_FLOAT);

	//创建GUI
	m_GUI = m_Device->CreateGUI(m_Window->GetWindowHandle(), m_SwapChain->GetRenderTarget());
	Application::Get().WndProcHandler += WndProcEvent::slot(&GUI::WndProcHandler, m_GUI);

	//LoadScene(L"C:\\Code\\DX12New\\Assets\\Models\\MaterialMesh.FBX");
	//LoadScene(L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj");
	m_LoadingTask = std::async(std::launch::async, std::bind(&Lesson5::LoadScene, this, L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj"));

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue.GetCommandList();

	//加载环境贴图
	auto hdrMap = commandList->LoadTextureFromFile(L"C:\\Code\\DX12New\\Assets\\Textures\\newport_loft.hdr", false);
	D3D12_RESOURCE_DESC desc = hdrMap->GetResourceDesc();
	desc.Height = 1024;
	desc.Width = 1024;
	desc.DepthOrArraySize = 6;
	desc.MipLevels = 6;

	m_CubeMap = m_Device->CreateTexture(desc, true);
	
	commandList->PanoToCubeMap(m_CubeMap, hdrMap);
	MaterialProperties Material = Material::Black;

	//构建模型
	//m_Sphere = commandList->LoadSceneFromFile(L"C:\\Code\\DX12New\\Assets\\Models\\sphere.fbx");//MeshHelper::CreateSphere(commandList, 1.0f);
	m_Sphere = MeshHelper::CreateSphere(commandList, 1.0f, 16);
	m_Cone = MeshHelper::CreateCone(commandList, 0.1f, 0.2f);
	m_Cube = MeshHelper::CreateCube(commandList, 5000.0f, true);
	m_Axis = commandList->LoadSceneFromFile(L"C:\\Code\\DX12New\\Assets\\Models\\axis_of_evil.nff");

	m_Cube->GetRootNode()->GetMesh()->GetMaterial()->SetTexture(Material::TextureType::Diffuse, m_CubeMap);

	//创建PSO
	m_UnlitPso = std::make_unique<EffectPSO>(m_Device, false, false);

	m_NormalVisualizePso = std::make_unique<NormalVisualizePSO>(m_Device);

	m_GBufferPso = std::make_unique<DeferredGBufferPSO>(m_Device, false);
	m_GBufferDecalPso = std::make_unique<DeferredGBufferPSO>(m_Device, true);

	m_DeferredLightingPso = std::make_unique<DeferredLightingPSO>(m_Device, true);

	m_SkyBoxPso = std::make_unique<SkyCubePSO>(m_Device);
	m_PrefilterPso = std::make_unique<SkyCubePSO>(m_Device, false, true);

	m_LDRPSO = std::make_unique<FinalLDRPSO>(m_Device);

	//构建灯光
	BuildLighting(4, 1, 1);
	BuildCubemapCamera();

	//执行命令列表
	auto fence = commandQueue.ExecuteCommandList(commandList);

	CreateGBufferRT();

	//创建渲染目标
	// HDR
	// Create a color buffer with sRGB for gamma correction.
	{
		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

		DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		colorDesc.MipLevels = 1;
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.4f;
		colorClearValue.Color[1] = 0.6f;
		colorClearValue.Color[2] = 0.9f;
		colorClearValue.Color[3] = 1.0f;

		std::shared_ptr<Texture> colorTexture;

		colorTexture = m_Device->CreateTexture(colorDesc, false, &colorClearValue);

		colorTexture->SetName(L"Color Render Target");

		m_HDRRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);

		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
#if USE_REVERSE_Z
		depthClearValue.DepthStencil = { 0.0f, 0 };
#else
		depthClearValue.DepthStencil = { 1.0f, 0 };
#endif

		auto depthTexture = m_Device->CreateTexture(depthDesc, false, &depthClearValue);
		depthTexture->SetName(L"Depth Render Target");

		m_HDRRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	}

	//LDR
	{
		DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

		DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, 1,
			0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		colorDesc.MipLevels = 1;
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.4f;
		colorClearValue.Color[1] = 0.6f;
		colorClearValue.Color[2] = 0.9f;
		colorClearValue.Color[3] = 1.0f;

		std::shared_ptr<Texture> colorTexture;

		colorTexture = m_Device->CreateTexture(colorDesc, false, &colorClearValue);

		colorTexture->SetName(L"Color Render Target");

		m_LDRRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);

		//深度图暂时不需要
			auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, 1,
				0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
#if USE_REVERSE_Z
		depthClearValue.DepthStencil = { 0.0f, 0 };
#else
		depthClearValue.DepthStencil = { 1.0f, 0 };
#endif

		auto depthTexture = m_Device->CreateTexture(depthDesc, false, &depthClearValue);
		depthTexture->SetName(L"Depth Render Target");

		m_LDRRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	}

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

	//更新灯光数据
	m_DeferredLightingPso->SetPointLights(m_PointLights);
	m_DeferredLightingPso->SetDirectionalLights(m_DirectionalLights);
	m_DeferredLightingPso->SetSpotLights(m_SpotLights);

	Vector4 cameraPoint = m_Camera.GetFocalPoint();

	m_Axis->GetRootNode()->SetScale(Vector4(0.3, 0.3, 0.3, 0));
	m_Axis->GetRootNode()->SetPosition(Vector4(0,0,0,0));
	//m_Axis->GetRootNode()->SetPosition(cameraPoint);
	//m_Axis->GetRootNode()->SetScale(Vector4(0.1, 0.1, 0.1, 0));

	m_SwapChain->WaitForSwapChain();

	//处理输入
	Application::Get().ProcessInput();
	m_CameraController.Update(e);

	OnRender();
}

void Lesson5::OnResize(ResizeEventArgs& e)
{
	m_Width = std::max(1, e.Width);
	m_Height = std::max(1, e.Height);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	float aspect = m_Width / (float)m_Height;
	m_Camera.SetProjection(45.0f, aspect, 0.1f, 10000.0f);

	m_HDRRenderTarget.Resize(m_Width, m_Height);
	m_GBufferRenderTarget.Resize(m_Width, m_Height);

	//LDR渲染目标
	m_LDRRenderTarget.Resize(m_Width, m_Height);

	//预计算立方体贴图卷积的RT
	RenderTarget m_IrradianceRenderTarget;

	m_SwapChain->Resize(m_Width, m_Height);
}

void Lesson5::OnRender()
{
	m_Window->SetFullScreen(m_Fullscreen);
	//std::cout << m_SpotLights.size() << std::endl;
	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto  commandList = commandQueue.GetCommandList();

	float DepthClearValue;
#if USE_REVERSE_Z
	DepthClearValue = 0.0f;
#else
	DepthClearValue = 1.0f;
#endif

	//进行预计算光照,只在第一次循环执行一次
	static bool isfirst = true;
	if (isfirst)
	{
		PreIrradiance(commandList);
		Prefilter(commandList);
		IntegrateBRDF(commandList);
		isfirst = false;
	}

	const auto& rendertarget = m_IsLoading ? m_SwapChain->GetRenderTarget() : m_GBufferRenderTarget;

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	if (m_IsLoading)
	{
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color0), clearColor);
	}
	else
	{
		//创建Pass
		SceneVisitor opaquePass(*commandList, m_Camera, *m_GBufferPso, false);
		SceneVisitor transparentPass(*commandList, m_Camera, *m_GBufferDecalPso, true);
		SceneVisitor unlitPass(*commandList, m_Camera, *m_UnlitPso, false);
		SceneVisitor skyBoxPass(*commandList, m_Camera, *m_SkyBoxPso, false);
		SceneVisitor LinePass(*commandList, m_Camera, *m_NormalVisualizePso, false);
	
		//清空缓冲区
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color1), clearColor);
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color2), clearColor);
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color3), clearColor);
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color4), clearColor);
		commandList->ClearDepthStencilTexture(rendertarget.GetTexture(AttachmentPoint::DepthStencil),D3D12_CLEAR_FLAG_DEPTH, DepthClearValue);
		//设置命令列表
		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(rendertarget);

		//渲染场景(GBUFFER  PASS)
		m_Scene->Accept(opaquePass);
		m_Axis->Accept(opaquePass);
		m_Scene->Accept(transparentPass);

		DrawSphere(opaquePass, false);
		
		//计算光照(PBR PASS)
		commandList->ClearTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH, DepthClearValue);
		commandList->SetRenderTarget(m_HDRRenderTarget);

		//获取GBuffer贴图
		std::vector<std::shared_ptr<Texture>> gBufferTexture;
		gBufferTexture.resize(DeferredLightingPSO::NumTextures);
		gBufferTexture[DeferredLightingPSO::AlbedoText] = m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color0);
		gBufferTexture[DeferredLightingPSO::NormalText] = m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color1);
		gBufferTexture[DeferredLightingPSO::ORMText] = m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color2);
		gBufferTexture[DeferredLightingPSO::EmissiveText] = m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color3);
		gBufferTexture[DeferredLightingPSO::WorldPosText] = m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color4);
		gBufferTexture[DeferredLightingPSO::IrradianceText] = m_IrradianceRenderTarget.GetTexture(AttachmentPoint::Color0);
		gBufferTexture[DeferredLightingPSO::PrefilterText] = m_PrefilterRenderTarget.GetTexture(AttachmentPoint::Color0);
		gBufferTexture[DeferredLightingPSO::IntegrateBRDFText] = m_IntegrateBRDFRenderTarget.GetTexture(AttachmentPoint::Color0);
		
		m_DeferredLightingPso->SetCameraPos(m_Camera.GetFocalPoint());
		m_DeferredLightingPso->SetTexture(gBufferTexture);
		m_DeferredLightingPso->Apply(*commandList);
		commandList->Draw(4);

		//将深度图拷贝到HDR渲染目标,渲染辅助附体
		commandList->CopyResource(m_HDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), m_GBufferRenderTarget.GetTexture(AttachmentPoint::DepthStencil));

		commandList->TransitionBarrier(m_HDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->FlushResourceBarriers();

		
	
		DrawLightMesh(unlitPass);
		m_Cube->Accept(skyBoxPass);
		//DrawSphere(LinePass, true);

		//HDR转LDR
		commandList->ClearTexture(m_LDRRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(m_LDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH, DepthClearValue);
		commandList->SetRenderTarget(m_LDRRenderTarget);
		m_LDRPSO->SetTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0));
		m_LDRPSO->Apply(*commandList);
		commandList->Draw(4);

		const auto& rt = m_SwapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
		commandList->CopyResource(rt, m_LDRRenderTarget.GetTexture(AttachmentPoint::Color0));
	}
	OnGUI(commandList, m_SwapChain->GetRenderTarget());

	commandQueue.ExecuteCommandList(commandList);

	m_SwapChain->Present();
}

void Lesson5::OnKeyPressed(KeyEventArgs& e)
{
	if (!ImGui::GetIO().WantCaptureKeyboard)
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
		case KeyCode::R:
			m_CameraController.ResetView();
			break;
		case KeyCode::O:
			if (e.Control)
			{
				OpenFile();
			}
			break;
		}
	}

}

void Lesson5::OnKeyReleased(KeyEventArgs& e)
{
	if (!ImGui::GetIO().WantCaptureKeyboard)
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
		}
	}
}
	
void Lesson5::OnMouseMoved(MouseMotionEventArgs& e)
{
	if (!ImGui::GetIO().WantCaptureMouse){}
}

void Lesson5::OnDPIScaleChanged(DPIScaleEventArgs& e)
{
	m_GUI->SetScaling(e.DPIScale);
}

void Lesson5::OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget)
{
	m_GUI->NewFrame();
	static bool show_demo_window = false;
	static bool showDetail = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	//进度条
	if (m_IsLoading)
	{
		ImGui::SetNextWindowPos(ImVec2(m_Window->GetWidth() / 2.0f, m_Window->GetHeight() / 2.0f), 0, ImVec2(0.5f, 0.5f));

		ImGui::SetNextWindowSize(ImVec2(m_Window->GetWidth() / 2.0f, 0.0f));

		ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

		ImGui::ProgressBar(m_LoadingProgress);
		ImGui::Text(m_LoadingText.c_str());

		if (!m_CancelLoading)
		{
			if (ImGui::Button("cancel"))
			{
				m_CancelLoading = true;
			}
		}
		else
		{
			ImGui::Text("Cancel Loading....");
		}

		ImGui::End();
	}
	
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open file...", "Ctrl+O", nullptr, !m_IsLoading))
			{
				m_ShowFileOpenDialog = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Esc"))
			{
				Application::Get().Stop();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("show_demo", nullptr, &show_demo_window);
			ImGui::MenuItem("Detail", nullptr, &showDetail);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			bool vSync = m_SwapChain->GetVSync();
			if (ImGui::MenuItem("V-Sync", "V", &vSync))
			{
				m_SwapChain->SetVSync(vSync);
			}

			bool fullscreen = m_Window->IsFullScreen();
			if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
			{
				m_Fullscreen = fullscreen;
			}

			ImGui::MenuItem("Animate Lights", "Space", &m_AnimateLights);

			bool invertY = m_CameraController.IsInverseY();
			if (ImGui::MenuItem("Inverse Y", nullptr, &invertY))
			{
				m_CameraController.SetInverseY(invertY);
			}
			if (ImGui::MenuItem("Reset view", "R"))
			{
				m_CameraController.ResetView();
			}

			ImGui::EndMenu();
		}

		char buffer[256];
		{
			sprintf_s(buffer, _countof(buffer), "FPS: %.2f (%.2f ms)  ", m_FPS, 1.0 / m_FPS * 1000.0);
			auto fpsTextSize = ImGui::CalcTextSize(buffer);
			ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x);
			ImGui::Text(buffer);
		}

		ImGui::EndMainMenuBar();
	}

	if (show_demo_window)
	{ 
		ImGui::ShowDemoWindow(&show_demo_window);	
	}
	
	if (showDetail)
	{
		GUILayout(&showDetail);
	}
	

	// Rendering
	m_GUI->Render(commandList, renderTarget);
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
		scene->GetRootNode()->SetPosition(Vector4(0, -10, 0, 0));
		//scene->GetRootNode()->SetRotation(Vector4(0, -10, 0, 0));
		scene->GetRootNode()->SetScale(Vector4(0.5, 0.5, 0.5, 0));

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

void Lesson5::BuildLighting(int numPointLights, int numSpotLights, int numDirectionalLights)
{
	static const XMVECTORF32 LightColors[] = { Colors::Red,     Colors::Green,  Colors::Blue,   Colors::Cyan,
											   Colors::Magenta, Colors::Yellow, Colors::Purple, Colors::White };

	static float lightAnimTime = 0.0f;
	if (m_AnimateLights)
	{
		//lightAnimTime += static_cast<float>(e.DeltaTime) * 0.5f * XM_PI;
	}

	// Spin the lights in a circle.
	const float radius = 1.0f;
	// Offset angle for light sources.
	float pointLightOffset = numPointLights > 0 ? 2.0f * XM_PI / numPointLights : 0;
	float spotLightOffset = numSpotLights > 0 ? 2.0f * XM_PI / numSpotLights : 0;
	float directionalLightOffset = numDirectionalLights > 0 ? 2.0f * XM_PI / numDirectionalLights : 0;

	// Setup the lights.
	m_PointLights.resize(numPointLights);
	for (int i = 0; i < numPointLights; ++i)
	{
		PointLight& l = m_PointLights[i];

		float angle = lightAnimTime + pointLightOffset * i;

		if (i == 2)
		{
			l.PositionWS = { -8 , -8, -5, 1.0f };
		}
		else if (i == 0)
		{
			l.PositionWS = { 8 , 8, -5, 1.0f };
		}
		else if (i == 3)
		{
			l.PositionWS = { -8 , 8, -5, 1.0f };
		}
		else
		{
			l.PositionWS = { 8 , -8, -5, 1.0f };
		}
		

		XMVECTOR positionWS = XMLoadFloat4(&l.PositionWS);
		XMVECTOR positionVS = positionWS;
		XMStoreFloat4(&l.PositionVS, positionVS);

		l.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0);
		l.Instensity = 1.0f;
		l.LinearAttenuation = 0.08f;
		l.QuadraticAttenuation = 0.1f;
		l.Range = 1.0f;
	}

	m_SpotLights.resize(numSpotLights);
	for (int i = 0; i < numSpotLights; ++i)
	{
		SpotLight& l = m_SpotLights[i];

		float angle = lightAnimTime + spotLightOffset * i + pointLightOffset / 2.0;

		l.PositionWS = { 0, 8, 0, 1};
		l.PositionVS = l.PositionWS;

		Vector4 directionWS(0.0f, -0.8f, 0, 0);
		Vector4 directionVS = directionWS;
		XMStoreFloat4(&l.DirectionWS, directionWS);
		XMStoreFloat4(&l.DirectionVS, directionVS);

		l.Color = XMFLOAT4(LightColors[(i + numPointLights) % _countof(LightColors)]);
		l.Color = XMFLOAT4(10, 1, 1, 0);
		l.Range = 5.0f;
		l.SpotAngle = XMConvertToRadians(90.0f);
		l.Intensity = 10.0f;
		l.LinearAttenuation = 0.08f;
		l.QuadraticAttenuation = 0.0f;
	}

	m_DirectionalLights.resize(numDirectionalLights);
	for (int i = 0; i < numDirectionalLights; ++i)
	{
		DirectionalLight& l = m_DirectionalLights[i];

		float angle = lightAnimTime + directionalLightOffset * i;

		Vector4 directionWS = Vector4(1, 1, 1, 0);
		Vector4 directionVS = directionWS;

		XMStoreFloat4(&l.DirectionWS, directionWS);
		XMStoreFloat4(&l.DirectionVS, directionVS);

		l.Color = XMFLOAT4(0.9,0.9,0.6,0);
		l.Intensity = 1.0f;
	}
}

void Lesson5::DrawLightMesh(SceneVisitor& _pass)
{
	MaterialProperties lightMaterial = Material::Black;
	for (const auto& l : m_PointLights)
	{
		lightMaterial.Diffuse = l.Color;
		auto lightPos = Vector4(l.PositionWS);

		m_Sphere->GetRootNode()->SetPosition(lightPos);
		m_Sphere->GetRootNode()->SetScale(Vector4(0.5, 0.5, 0.5, 0));
		m_Sphere->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(lightMaterial);
		m_Sphere->Accept(_pass);
	}

	for (const auto& l : m_SpotLights)
	{
		lightMaterial.Diffuse = l.Color;
		Vector4 lightPos = Vector4(l.PositionWS);
		Vector4 lightDir = Vector4(l.DirectionWS);
		Vector4 up = Vector4(0, 1, 0, 0);

		// Rotate the cone so it is facing the Z axis.

		m_Cone->GetRootNode()->SetPosition(lightPos);
		m_Cone->GetRootNode()->SetScale(Vector4(5, 5, 5, 0));
		m_Cone->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(lightMaterial);
		m_Cone->Accept(_pass);
	}
}

void Lesson5::CreateGBufferRT()
{
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	colorDesc.MipLevels = 1;

	//普通RT的格式
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	//坐标通道RT的格式
	D3D12_CLEAR_VALUE colorClearValue2;
	colorClearValue2.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	colorClearValue2.Color[0] = 0.4f;
	colorClearValue2.Color[1] = 0.6f;
	colorClearValue2.Color[2] = 0.9f;
	colorClearValue2.Color[3] = 1.0f;

	auto desc2 = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	desc2.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	auto desc3 = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_Width, m_Height, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	desc3.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	for (int i = 0; i < 5; i++)
	{
		std::shared_ptr<Texture> colorTexture;
		if (i == 0)
		{
			colorTexture = m_Device->CreateTexture(colorDesc, false, &colorClearValue);
		}
		else if (i == 4 || i == 1)
		{
			colorTexture = m_Device->CreateTexture(desc3, false, &colorClearValue2);
		}
		else
		{
			colorTexture = m_Device->CreateTexture(desc2, false, &colorClearValue);
		}
		colorTexture->SetName(L"Color Render Target");

		m_GBufferRenderTarget.AttachTexture(static_cast<AttachmentPoint>(i), colorTexture);
	}

	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
#if USE_REVERSE_Z
	depthClearValue.DepthStencil = { 0.0f, 0 };
#else
	depthClearValue.DepthStencil = { 1.0f, 0 };
#endif

	auto depthTexture = m_Device->CreateTexture(depthDesc, false, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");

	m_GBufferRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
}

void Lesson5::DrawSphere(SceneVisitor& _pass, bool isNormal)
{
	if (isNormal)
	{
		m_Sphere->GetRootNode()->GetMesh()->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	}
	else
	{
		m_Sphere->GetRootNode()->GetMesh()->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	int i = 0;
	for (int x = -5; x < 6; x++)
	{
		for (int y = -5; y < 6; y++)
		{
			m_Sphere->GetRootNode()->SetScale(Vector4(0.6, 0.6, 0.6, 0));
			m_Sphere->GetRootNode()->SetPosition(Vector4(x * 1.3, y * 1.3, 0, 0));
			float roughness = Clamp(((x + 5.0f) / 10.0f), 0.0f, 0.99f);
			float metallic = Clamp(((y + 5.0f) / 10.0f), 0.0f, 0.99f);
			m_Sphere->GetRootNode()->GetMesh()->GetMaterial()->SetORMColor(Vector4(1, roughness, metallic, 1.0f));
			m_Sphere->GetRootNode()->GetMesh()->GetMaterial()->SetDiffuseColor(Vector4(1.0,0,0,0));
			m_Sphere->Accept(_pass);
		}
	}
}

void Lesson5::BuildCubemapCamera()
{
	float x = 0;
	float y = 0;
	float z = 0;

	//创建指定位置处的CubeMap
	Vector4 center(x, y, z, 1.0f);//指定位置
	Vector4 worldUp(0.0f, 1.0f, 0.0f, 0.0f);//向上向量

	//CubeMap的6个Target向量
	Vector4 targets[6] =
	{
		Vector4(x + 1.0f, y, z , 1.0f), // +X
		Vector4(x - 1.0f, y, z , 1.0f), // -X
		Vector4(x, y + 1.0f, z , 1.0f), // +Y
		Vector4(x, y - 1.0f, z , 1.0f), // -Y
		Vector4(x, y, z + 1.0f , 1.0f), // +Z
		Vector4(x, y, z - 1.0f , 1.0f)  // -Z
	};

	//6个Up向量，Y轴使用“特殊”的Up向量
	Vector4 ups[6] =
	{
		Vector4(0.0f, 1.0f, 0.0f	, 1.0f),  // +X
		Vector4(0.0f, 1.0f, 0.0f	, 1.0f),  // -X
		Vector4(0.0f, 0.0f, -1.0f	, 1.0f), // +Y
		Vector4(0.0f, 0.0f, +1.0f	, 1.0f), // -Y
		Vector4(0.0f, 1.0f, 0.0f	, 1.0f),	 // +Z
		Vector4(0.0f, 1.0f, 0.0f	, 1.0f)	 // -Z
	};

	//设置相机矩阵
	for (int i = 0; i < 6; i++)
	{
		m_CubeMapCamera[i].SetLookAt(center, targets[i], ups[i]);

		m_CubeMapCamera[i].SetProjection(90.0f, 1.0f, 0.1f, 1000.0f);
	}
}

void Lesson5::PreIrradiance(std::shared_ptr<CommandList> _commandList)
{
	auto cube = MeshHelper::CreateCube(_commandList, 10.0f, true);

	//创建渲染目标
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//RT资源描述
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, 32, 32, 6, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	colorDesc.MipLevels = 1;
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	//RT
	std::shared_ptr<Texture> colorTexture;
	colorTexture = m_Device->CreateTexture(colorDesc, true, &colorClearValue);
	_commandList->TransitionBarrier(colorTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	colorTexture->SetName(L"Color Render Target");
	m_IrradianceRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);

	//DS资源描述
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, 32, 32, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
#if USE_REVERSE_Z
	float DepthClearValue = 0.0f;
	depthClearValue.DepthStencil = { 0.0f, 0 };
#else
	float DepthClearValue = 1.0f;
	depthClearValue.DepthStencil = { 1.0f, 0 };
#endif

	//深度图
	auto depthTexture = m_Device->CreateTexture(depthDesc, false, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");
	m_IrradianceRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	//设置渲染用的视口
	_commandList->SetScissorRect(m_IrradianceRenderTarget.GetScissorRect());
	_commandList->SetViewport(m_IrradianceRenderTarget.GetViewport());
	
	m_PreCalPso = std::make_unique<SkyCubePSO>(m_Device, true);
	//渲染环境图
	cube->GetRootNode()->GetMesh()->GetMaterial()->SetTexture(Material::TextureType::Diffuse, m_CubeMap);
	for (int i = 0; i < 6; i++)
	{
		//设置渲染目标
		auto rtv = colorTexture->GetRenderTargetView(i);
		auto dsv = depthTexture->GetDepthStencilView();
		_commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &rtv, true, &dsv);

		_commandList->GetGraphicsCommandList()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		_commandList->ClearDepthStencilTexture(m_IrradianceRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH, DepthClearValue);
		SceneVisitor skyBoxPass(*_commandList, m_CubeMapCamera[i], *m_PreCalPso, false);
		cube->Accept(skyBoxPass);
	}

	m_CubeMap1 = m_Device->CreateTexture(m_IrradianceRenderTarget.GetTexture(AttachmentPoint::Color0)->GetResource(), true, &colorClearValue);

	//m_Cube->GetRootNode()->GetMesh()->GetMaterial()->SetTexture(Material::TextureType::Diffuse, m_CubeMap1);
}

void Lesson5::Prefilter(std::shared_ptr<CommandList> _commandList)
{
	auto cube = MeshHelper::CreateCube(_commandList, 10.0f, true);

	//创建渲染目标
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//RT资源描述
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, 128, 128, 6, 5, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	//RT
	std::shared_ptr<Texture> colorTexture;
	colorTexture = m_Device->CreateTexture(colorDesc, true, &colorClearValue);
	_commandList->TransitionBarrier(colorTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	colorTexture->SetName(L"Color Render Target");
	m_PrefilterRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);

	//DS资源描述
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, 128, 128, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
#if USE_REVERSE_Z
	float DepthClearValue = 0.0f;
	depthClearValue.DepthStencil = { 0.0f, 0 };
#else
	float DepthClearValue = 1.0f;
	depthClearValue.DepthStencil = { 1.0f, 0 };
#endif

	//深度图
	auto depthTexture = m_Device->CreateTexture(depthDesc, false, &depthClearValue);
	depthTexture->SetName(L"Depth Render Target");
	m_PrefilterRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	cube->GetRootNode()->GetMesh()->GetMaterial()->SetTexture(Material::TextureType::Diffuse, m_CubeMap);
	for (auto mip = 0u; mip < 5; mip++)
	{
		UINT width = 128 * std::pow(0.5, mip);
		UINT height = 128 * std::pow(0.5, mip);

		_commandList->SetScissorRect(CalRect(width, height));
		_commandList->SetViewport(CalViewport(width, height));

		float roughness = (float)mip / (float)(5 - 1);
		m_PrefilterPso->SetRoughness(roughness);

		for (auto j = 0u; j < 6; j++)
		{
			//	//设置渲染目标
			auto rtv = colorTexture->GetRenderTargetView(j * 5 + mip);
			auto dsv = depthTexture->GetDepthStencilView();
			_commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &rtv, true, &dsv);

			_commandList->GetGraphicsCommandList()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
			_commandList->ClearDepthStencilTexture(m_PrefilterRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH, DepthClearValue);
			SceneVisitor skyBoxPass(*_commandList, m_CubeMapCamera[j], *m_PrefilterPso, false);
			cube->Accept(skyBoxPass);
		}
	}
	m_PrefilterCubeMap = m_Device->CreateTexture(m_PrefilterRenderTarget.GetTexture(AttachmentPoint::Color0)->GetResource(), true, &colorClearValue);

	//m_Cube->GetRootNode()->GetMesh()->GetMaterial()->SetTexture(Material::TextureType::Diffuse, m_PrefilterCubeMap);
}

void Lesson5::IntegrateBRDF(std::shared_ptr<CommandList> _commandList)
{
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16_FLOAT;

	//RT资源描述
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, 512, 512, 1, 1, 1,
		0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	//RT
	std::shared_ptr<Texture> colorTexture;
	colorTexture = m_Device->CreateTexture(colorDesc, false, &colorClearValue);
	_commandList->TransitionBarrier(colorTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	colorTexture->SetName(L"Color Render Target");
	m_IntegrateBRDFRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);

	_commandList->ClearTexture(m_IntegrateBRDFRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);

	//设置命令列表
	_commandList->SetViewport(m_IntegrateBRDFRenderTarget.GetViewport());
	_commandList->SetScissorRect(m_IntegrateBRDFRenderTarget.GetScissorRect());
	_commandList->SetRenderTarget(m_IntegrateBRDFRenderTarget);
	_commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_IntegrateBRDFPSO = std::make_unique<IntegrateBRDFPSO>(m_Device);
	m_IntegrateBRDFPSO->Apply(*_commandList);
	_commandList->Draw(4);
}

void Lesson5::GUILayout(bool* _open)
{
	//std::cout << *_open << std::endl;
	ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	bool b = ImGui::Begin("Example: Simple layout", _open, ImGuiWindowFlags_MenuBar);
	if (b)
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Close")) *_open = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (m_Scene)
		{// Left
			static int selected = 0;
			{
				ImGui::BeginChild("left pane", ImVec2(150, 0), true);

				for (int i = 0; i < m_Scene->GetRootNode()->GetSize(); i++)
				{
					// FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
					char label[128];
					sprintf_s(label, "MyObject %d", i);
					if (ImGui::Selectable(label, selected == i))
						selected = i;
				}
				ImGui::EndChild();
			}
			ImGui::SameLine();
			// Right
			{
				ImGui::BeginGroup();
				ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
				ImGui::Text("MyObject: %d", selected);
				ImGui::Separator();
				if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("Material"))
					{
						static float color[4];
						ImGui::ColorEdit4("color", color);
						Vector4 e = Vector4(color);
						m_Scene->GetRootNode()->GetMesh(selected)->GetMaterial()->SetORMColor(e);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Details"))
					{
						static float pos[4];
						static float sacle[4];
						static float rotation[4];
						BindTransform(pos, rotation, sacle, m_Scene->GetRootNode());

						ImGui::DragFloat3("pos", pos);
						ImGui::DragFloat3("rotation", rotation);
						ImGui::DragFloat3("sacle", sacle);

						m_Scene->GetRootNode()->SetPosition(Vector4(pos));
						m_Scene->GetRootNode()->SetRotation(Vector4(rotation));
						m_Scene->GetRootNode()->SetScale(Vector4(sacle));

						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndChild();
				ImGui::EndGroup();
			}
		}
	}
	ImGui::End();
}

void Lesson5::BindTransform(float* _pos, float* _rotation, float* _scale, std::shared_ptr<SceneNode> _node)
{
	_pos[0] = _node->GetPosition().GetX();
	_pos[1] = _node->GetPosition().GetY();
	_pos[2] = _node->GetPosition().GetZ();

	_rotation[0] = _node->GetRotation().GetX();
	_rotation[1] = _node->GetRotation().GetY();
	_rotation[2] = _node->GetRotation().GetZ();

	_scale[0] = _node->GetScale().GetX();
	_scale[1] = _node->GetScale().GetY();
	_scale[2] = _node->GetScale().GetZ();
}

