#include "SwapChain.h"
#include "Device.h"
#include "Texture.h"
#include "D3D12LibPCH.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ResourceStateTracker.h"
#include "Adapter.h"


void SwapChain::SetFullScreen(bool _fullScreen)
{
	if (m_FullScreen != _fullScreen)
	{
		m_FullScreen = _fullScreen;
		// TODO:
		//        m_dxgiSwapChain->SetFullscreenState()
	}
}

void SwapChain::WaitForSwapChain()
{
	DWORD result = ::WaitForSingleObjectEx(m_hFrameLatencyWaitableObject, 1000, TRUE);//等待一秒
}

void SwapChain::Resize(uint32_t _width, uint32_t _height)
{
	//只有判断窗口大小发生过变化才会执行
	if (m_Width != _width || m_Height != _height)
	{
		//限制大小最小为1
		m_Width = std::max(1u, _width);
		m_Height = std::max(1u, _height);

		//刷新命令队列
		m_Device.Flush();

		//释放后台缓冲区和渲染目标
		m_RenderTarget.Resize(m_Width, m_Height);
		for (UINT i = 0; i < BufferCount; ++i)
		{
			//ResourceStateTracker::RemoveGlobalResourceState( m_BackBufferTextures[i]->GetD3D12Resource().Get(), true );
			m_BackBufferTextures[i].reset();
		}

		//更改大小
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(BufferCount, m_Width, m_Height, swapChainDesc.BufferDesc.Format,
			swapChainDesc.Flags));

		//获取当前的后台缓冲区索引
		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		//更新RTV
		UpdateRenderTargetViews();
	}
}

const RenderTarget& SwapChain::GetRenderTarget() const
{
	//将当前的后台缓冲区附加到渲染目标的颜色0接口上
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, m_BackBufferTextures[m_CurrentBackBufferIndex]);

	return m_RenderTarget;
}

UINT SwapChain::Present(const std::shared_ptr<Texture>& _texture /*= nullptr*/)
{
	auto commandList = m_CommandQueue.GetCommandList();
	auto backbuffer = m_BackBufferTextures[m_CurrentBackBufferIndex];

	//判断是否需要将纹理复制到后台缓冲
	if (_texture)
	{
		if (_texture->GetResourceDesc().SampleDesc.Count > 1)
		{
			commandList->ResolveSubresource(backbuffer, _texture);
		}
		else
		{
			commandList->CopyResource(backbuffer, _texture);
		}
	}

	//转换资源状态为呈现
	commandList->TransitionBarrier(backbuffer, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandQueue.ExecuteCommandList(commandList);

	//设置显示状态
	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_TearingSupported && !m_FullScreen && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	//获取围栏值
	m_FenceValues[m_CurrentBackBufferIndex] = m_CommandQueue.Signal();

	//获取下一个后台缓冲索引
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	//等待围栏值到达
	auto fenceValue = m_FenceValues[m_CurrentBackBufferIndex];
	m_CommandQueue.WaitForFenceValue(fenceValue);

	//释放过时的描述符
	m_Device.ReleaseStaleDescriptors();

	return m_CurrentBackBufferIndex;
}

SwapChain::SwapChain(Device& _device, HWND _hwnd, DXGI_FORMAT _rendertargetFormat /*= DXGI_FORMAT_R10G10B10A2_UNORM*/)
	:m_Device(_device), 
	m_HWnd(_hwnd), 
	m_RenderTargetFormat(_rendertargetFormat),
	m_CommandQueue(m_Device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)),
	m_FenceValues{0},
	m_Width(0),
	m_Height(0),
	m_VSync(true),
	m_TearingSupported(false),
	m_FullScreen(false)
{
	assert(_hwnd);

	//获取命令队列
	auto commandQueue = m_CommandQueue.GetCommandQueue();
	//获取适配器
	auto adapter = m_Device.GetAdapter();
	auto dxgiAdapter = adapter->GetDXGIAdapter();

	//创建工厂类
	Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;

	ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

	ThrowIfFailed(dxgiFactory.As(&dxgiFactory5));

	//检查对防撕裂的支持
	bool allowTearing = false;
	if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(bool))))
	{
		m_TearingSupported = (allowTearing == true);
	}

	//计算窗口大小
	RECT windowRect;
	::GetClientRect(_hwnd, &windowRect);

	m_Width = windowRect.right - windowRect.left;
	m_Height = windowRect.bottom - windowRect.top;

	//创建交换链描述
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = m_Width;
	desc.Height = m_Height;
	desc.Format = m_RenderTargetFormat;
	desc.Stereo = FALSE;
	desc.SampleDesc = { 1, 0 };
	desc.BufferCount = BufferCount;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	//创建交换链
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory5->CreateSwapChainForHwnd(
		commandQueue.Get(),
		m_HWnd,
		&desc,
		nullptr,
		nullptr, &swapChain1));

	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	//关闭ALT+ENTER全屏的功能
	ThrowIfFailed(dxgiFactory5->MakeWindowAssociation(m_HWnd, DXGI_MWA_NO_ALT_ENTER));
	
	//设置最大帧延迟
	m_SwapChain->SetMaximumFrameLatency(BufferCount - 1);

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	//获取交换链可等待对象
	m_hFrameLatencyWaitableObject = m_SwapChain->GetFrameLatencyWaitableObject();

	UpdateRenderTargetViews();
}

SwapChain::~SwapChain()
{

}

void SwapChain::UpdateRenderTargetViews()
{
	for (UINT i = 0; i < BufferCount; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

		m_BackBufferTextures[i] = m_Device.CreateTexture(backBuffer);

		m_BackBufferTextures[i]->SetName(L"Backbuffer[" + std::to_wstring(i) + L"]");
	}
}
