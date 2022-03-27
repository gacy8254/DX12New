#include "Lesson5.h"
#include "D3D12LibPCH.h"
#include <ShObjIdl.h>  // For IFileOpenDialog
#include <shlwapi.h>
#include "helpers.h"
#include <regex>
#include <iostream>
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

	auto& commandQueue = m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue.GetCommandList();

	// Start the loading task to perform async loading of the scene file.
	LoadScene(L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj");
	//m_LoadingTask = std::async(std::launch::async, std::bind(&Lesson5::LoadScene, this,L"C:\\Code\\DX12New\\Assets\\Models\\crytek-sponza\\sponza_nobanner.obj"));

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
	OnRender();
}

void Lesson5::OnResize(ResizeEventArgs& e)
{
	m_Width = std::max(1, e.Width);
	m_Height = std::max(1, e.Height);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	m_RenderTarget.Resize(m_Width, m_Height);

	m_SwapChain->Resize(m_Width, m_Height);
}

void Lesson5::OnRender()
{
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
		commandList->ClearTexture(rendertarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(rendertarget.GetTexture(AttachmentPoint::DepthStencil),D3D12_CLEAR_FLAG_DEPTH);
		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(rendertarget);

		auto swapChainBackBuffer = m_SwapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
		auto msaaRenderTarget = m_RenderTarget.GetTexture(AttachmentPoint::Color0);

		commandList->ResolveSubresource(swapChainBackBuffer, msaaRenderTarget);
	}
	
	commandQueue.ExecuteCommandList(commandList);

	m_SwapChain->Present();
}

void Lesson5::OnKeyPressed(KeyEventArgs& e)
{

}

void Lesson5::OnKeyReleased(KeyEventArgs& e)
{

}

void Lesson5::OnMouseMoved(MouseMotionEventArgs& e)
{

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

	m_IsLoading = true;

	return m_Scene != nullptr;
}

void Lesson5::OpenFile()
{

}

bool Lesson5::LoadingProgress(float _loadingProgress)
{
	return true;
}
