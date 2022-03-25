#include "SwapChain.h"

SwapChain::SwapChain(Device& _device, HWND _hwnd, DXGI_FORMAT _rendertargetFormat /*= DXGI_FORMAT_R10G10B10A2_UNORM*/)
	:m_Device(_device), m_hWnd(_hwnd), m_RenderTargetFormat(_rendertargetFormat)
{
//	Application& app = Application::Get();
//
//	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain4;
//	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;
//
//	UINT createFactoryFlags = 0;
//#if defined(_DEBUG)
//	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
//#endif
//
//	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
//
//	DXGI_SWAP_CHAIN_DESC1 desc = {};
//	desc.Width = m_Width;
//	desc.Height = m_Height;
//	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	desc.Stereo = FALSE;
//	desc.SampleDesc = { 1, 0 };
//	desc.BufferCount = m_BufferCount;
//	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	desc.Scaling = DXGI_SCALING_STRETCH;
//	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//	desc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
//
//
//	ID3D12CommandQueue* pCommandQueue = app.GetCommandQueue()->GetCommandQueue().Get();
//	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
//	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
//		pCommandQueue,
//		m_HWND,
//		&desc,
//		nullptr,
//		nullptr, &swapChain1));
//
//	//关闭ALT+ENTER全屏的功能
//	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_HWND, DXGI_MWA_NO_ALT_ENTER));
//	ThrowIfFailed(swapChain1.As(&swapChain4));
//
//	m_CurrentBackBufferIndex = swapChain4->GetCurrentBackBufferIndex();
//
//	return swapChain4;
}
