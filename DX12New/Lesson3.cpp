//#include "Lesson3.h"
//#include "Application.h"
//#include "CommandList.h"
//#include "CommandQueue.h"
//#include "helpers.h"
//#include "Light.h"
//#include "Material.h"
//#include "Window.h"
//#include <iostream>
//#include <wrl.h>
//using namespace Microsoft::WRL;
//
//#include "d3dx12.h"
//#include <d3dcompiler.h>
//#include <DirectXColors.h>
//using namespace DirectX;
//
//#include <algorithm> // For std::min and std::max.
//#if defined(min)
//#undef min
//#endif
//
//#if defined(max)
//#undef max
//#endif
//
//enum TonemapMethod : uint32_t
//{
//	TM_Linear,
//	TM_Reinhard,
//	TM_ReinhardSq,
//	TM_ACESFilmic,
//};
//
//struct TonemapParameters
//{
//	TonemapParameters()
//		: TonemapMethod(TM_Reinhard)
//		, Exposure(0.0f)
//		, MaxLuminance(1.0f)
//		, K(1.0f)
//		, A(0.22f)
//		, B(0.3f)
//		, C(0.1f)
//		, D(0.2f)
//		, E(0.01f)
//		, F(0.3f)
//		, LinearWhite(11.2f)
//		, Gamma(2.2f)
//	{}
//
//	// The method to use to perform tonemapping.
//	TonemapMethod TonemapMethod;
//	// Exposure should be expressed as a relative exposure value (-2, -1, 0, +1, +2 )
//	float Exposure;
//
//	// The maximum luminance to use for linear tonemapping.
//	float MaxLuminance;
//
//	// Reinhard constant. Generally this is 1.0.
//	float K;
//
//	// ACES Filmic parameters
//	// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
//	float A; // Shoulder strength
//	float B; // Linear strength
//	float C; // Linear angle
//	float D; // Toe strength
//	float E; // Toe Numerator
//	float F; // Toe denominator
//	// Note E/F = Toe angle.
//	float LinearWhite;
//	float Gamma;
//};
//
//TonemapParameters g_TonemapParameters;
//
//struct Mat
//{
//	XMMATRIX ModelMatrix;
//	XMMATRIX ModelViewMatrix;
//	XMMATRIX InverseTransposeModelMatrix;
//	XMMATRIX ModelViewProjMatrix;
//};
//
//struct LightProperties
//{
//	uint32_t NumPointLights;
//	uint32_t NumSpotLights;
//};
//
//enum RootParameters
//{
//	MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
//	MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
//	LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );
//	PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
//	SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
//	Textures,           // Texture2D DiffuseTexture : register( t2 );
//	NumRootParameters
//};
//
//// Builds a look-at (world) matrix from a point, up and direction vectors.
//XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR _position, FXMVECTOR _direction, FXMVECTOR  _up)
//{
//	assert(!XMVector3Equal(_direction, XMVectorZero()));
//	assert(!XMVector3IsInfinite(_direction));
//	assert(!XMVector3Equal(_up, XMVectorZero()));
//	assert(!XMVector3IsInfinite(_up));
//
//	XMVECTOR R2 = XMVector3Normalize(_direction);
//	XMVECTOR R0 = XMVector3Cross(_up, R2);
//	R0 = XMVector3Normalize(R0);
//
//	XMVECTOR R1 = XMVector3Cross(R2, R0);
//
//	XMMATRIX M(R0, R1, R2, _position);
//
//	return M;
//}
//
//Lesson3::Lesson3(const std::wstring& _name, int _width, int _height, bool _vSync)
//	:super(_name, _width, _height, _vSync),
//	m_Rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
//	m_Viewport(CD3DX12_VIEWPORT(0.0F, 0.0F, static_cast<float>(_width), static_cast<float>(_height))),
//	m_Forward(0)
//	, m_Backward(0)
//	, m_Left(0)
//	, m_Right(0)
//	, m_Up(0)
//	, m_Down(0)
//	, m_Pitch(0)
//	, m_Yaw(0)
//	, m_AnimaeLights(false)
//	, m_Shift(false)
//	, m_Width(_width)
//	, m_Height(_height)
//{
//	XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
//	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
//	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);
//
//	m_Camera.SetLookAt(cameraPos, cameraTarget, cameraUp);
//	float aspect = m_Width / (float)m_Height;
//	m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);
//
//	m_pAlignedCameraDate = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
//
//	m_pAlignedCameraDate->m_InitialCamPos = m_Camera.GetTranslation();
//	m_pAlignedCameraDate->m_InitialCamRot = m_Camera.GetRotation();
//	m_pAlignedCameraDate->m_InitialFov = m_Camera.GetFov();
//}
//
//Lesson3::~Lesson3()
//{
//	_aligned_free(m_pAlignedCameraDate);
//}
//
//bool Lesson3::LoadContent()
//{
//	return true;
//}
//
//void Lesson3::UnLoadContent()
//{
//
//}
//
//void Lesson3::OnUpdate(UpdateEventArgs& _e)
//{
//	static uint64_t frameCount = 0;
//	static double totalTime = 0.0;
//
//	super::OnUpdate(_e);
//
//	//计算帧数
//	if (totalTime > 1.0f)
//	{
//		double fps = frameCount / totalTime;
//
//		//system("cls");
//		std::cout << "FPS: " << fps << std::endl;
//
//		frameCount = 0;
//		totalTime = 0.0f;
//	}
//
//}
//
//void XM_CALLCONV ComputerMatrices(FXMMATRIX _model, CXMMATRIX _view, CXMMATRIX _viewProj, Mat& _mat)
//{
//	_mat.ModelMatrix = _model;
//	_mat.ModelViewMatrix = _model * _view;
//	_mat.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, _mat.ModelViewMatrix));
//	_mat.ModelViewProjMatrix = _model * _viewProj;
//}
//
////void Lesson3::OnRender(RenderEventArgs& _e)
////{
////
////	super::OnRender(_e);
////
////		//获取所需的对象
////	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
////	auto commandList = commandQueue->GetCommandList();
////
////	//清除缓冲区
////	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
////	commandList->ClearTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
////	commandList->ClearDepthStencilTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
////
////	//设置视口
////	commandList->SetViewport(m_Viewport);
////	commandList->SetScissorRect(m_Rect);
////	//设置渲染目标
////	commandList->SetRenderTarget(m_HDRRenderTarget);
////	
////	//渲染天空盒
////	{
////		XMMATRIX translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
////		XMMATRIX rotationMatrix = XMMatrixIdentity();
////		XMMATRIX scaleMatrix = XMMatrixScaling(400.0f, 400.0f, 400.0f);
////		XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
////		auto viewMat = m_Camera.GetViewMatrix();//XMMatrixTranspose(XMMatrixRotationQuaternion(m_Camera.GetRotation()));
////		auto projMat = m_Camera.GetProjMatrix();
////		auto viewProjMat = worldMatrix * viewMat * projMat;
////
////		commandList->SetPipelineState(m_SkyPSO);
////		commandList->SetGraphicsRootSignature(m_SkyboxRootSignature);
////
////		commandList->SetGraphics32BitConstants(0, viewProjMat);
////
////		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
////		srvDesc.Format = m_GraceCathedralCubemap.GetResourceDesc().Format;
////		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
////		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
////		srvDesc.TextureCube.MipLevels = (UINT)-1;//使用全部Mips
////
////		commandList->SetShaderResourceView(1, 0, m_GraceCathedralCubemap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &srvDesc);
////
////		//m_SkyBox->Draw(*commandList);
////	}
////
////	commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
////	commandList->SetViewport(m_pWindow->GetRenderTarget().GetViewport());
////	commandList->SetPipelineState(m_SDRPSO);
////	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
////	commandList->SetGraphicsRootSignature(m_SDRRootSignature);
////	commandList->SetGraphics32BitConstants(0, g_TonemapParameters);
////	commandList->SetShaderResourceView(1, 0, m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
////	commandList->Draw(3);
////
////	commandQueue->ExecuteCommandList(commandList);
////
////}
//
//static bool g_AllowFullscreenToggle = true;
//
//void Lesson3::OnKeyPressed(KeyEventArgs& _e)
//{
//	super::OnKeyPressed(_e);
//
//	switch (_e.key)
//	{
//	case KeyCode::Escape:
//		Application::Get().Quit(0);
//		break;
//	case KeyCode::Enter:
//		if (_e.alt)
//		{
//	case KeyCode::F11:
//		m_pWindow->ToggleFullScreen();
//		break;
//		}
//	case KeyCode::V:
//		m_pWindow->ToggleVSync();
//		break;
//	case KeyCode::R:
//		// Reset camera transform
//		m_Camera.SetTranslation(m_pAlignedCameraDate->m_InitialCamPos);
//		m_Camera.SetRotation(m_pAlignedCameraDate->m_InitialCamRot);
//		m_Camera.SetFov(m_pAlignedCameraDate->m_InitialFov);
//		m_Pitch = 0.0f;
//		m_Yaw = 0.0f;
//		break;
//	case KeyCode::Up:
//	case KeyCode::W:
//		m_Forward = 1.0f;
//		break;
//	case KeyCode::Left:
//	case KeyCode::A:
//		m_Left = 1.0f;
//		break;
//	case KeyCode::Down:
//	case KeyCode::S:
//		m_Backward = 1.0f;
//		break;
//	case KeyCode::Right:
//	case KeyCode::D:
//		m_Right = 1.0f;
//		break;
//	case KeyCode::Q:
//		m_Down = 1.0f;
//		break;
//	case KeyCode::E:
//		m_Up = 1.0f;
//		break;
//	case KeyCode::Space:
//		m_AnimaeLights = !m_AnimaeLights;
//		break;
//	case KeyCode::ShiftKey:
//		m_Shift = true;
//		break;
//	}
//}
//
//void Lesson3::OnKeyReleased(KeyEventArgs& _e)
//{
//	super::OnKeyReleased(_e);
//
//	/*switch (_e.key)
//	{
//	case KeyCode::Enter:
//		if (_e.alt)
//		{
//	case KeyCode::F11:
//		g_AllowFullscreenToggle = true;
//		}
//		break;
//	case KeyCode::Up:
//	case KeyCode::W:
//		m_Forward = 0.0f;
//		break;
//	case KeyCode::Left:
//	case KeyCode::A:
//		m_Left = 0.0f;
//		break;
//	case KeyCode::Down:
//	case KeyCode::S:
//		m_Backward = 0.0f;
//		break;
//	case KeyCode::Right:
//	case KeyCode::D:
//		m_Right = 0.0f;
//		break;
//	case KeyCode::Q:
//		m_Down = 0.0f;
//		break;
//	case KeyCode::E:
//		m_Up = 0.0f;
//		break;
//	case KeyCode::ShiftKey:
//		m_Shift = false;
//		break;
//	}*/
//}
//
//
//void Lesson3::OnMouseMoved(MouseMotionEventArgs& _e)
//{
//	super::OnMouseMoved(_e);
//
//	const float mouseSpeed = 0.1f;
//	if (_e.LeftButton)
//	{
//		m_Pitch += _e.RelY * mouseSpeed;
//
//		m_Pitch = Clamp(m_Pitch, -90.0f, 90.0f);
//
//		m_Yaw += _e.RelX * mouseSpeed;
//	}
//}
//
//void Lesson3::OnMouseButtonPressed(MouseButtonEventArgs& _e)
//{
//
//}
//
//void Lesson3::OnMouseButtonReleased(MouseButtonEventArgs& _e)
//{
//
//}
//
//void Lesson3::OnMouseWheel(MouseWheelEventArgs& _e)
//{
//}
//
//void Lesson3::OnResize(ResizeEventArgs& _e)
//{
//	super::OnResize(_e);
//
//	if (_e.Width != GetWidth() || _e.Height != GetHeight())
//	{
//		m_Width = std::max(1, _e.Width);
//		m_Height = std::max(1, _e.Height);
//
//		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
//
//		float aspect = m_Width / (float)m_Height;
//		m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);
//
//		m_SDRRenderTarget.Resize(m_Width, m_Height);
//	}
//}
//
//void Lesson3::OnWindowDestroy()
//{
//
//}
