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
	DWORD result = ::WaitForSingleObjectEx(m_hFrameLatencyWaitableObject, 1000, TRUE);//�ȴ�һ��
}

void SwapChain::Resize(uint32_t _width, uint32_t _height)
{
	//ֻ���жϴ��ڴ�С�������仯�Ż�ִ��
	if (m_Width != _width || m_Height != _height)
	{
		//���ƴ�С��СΪ1
		m_Width = std::max(1u, _width);
		m_Height = std::max(1u, _height);

		//ˢ���������
		m_Device.Flush();

		//�ͷź�̨����������ȾĿ��
		m_RenderTarget.Resize(m_Width, m_Height);
		for (UINT i = 0; i < BufferCount; ++i)
		{
			//ResourceStateTracker::RemoveGlobalResourceState( m_BackBufferTextures[i]->GetD3D12Resource().Get(), true );
			m_BackBufferTextures[i].reset();
		}

		//���Ĵ�С
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(BufferCount, m_Width, m_Height, swapChainDesc.BufferDesc.Format,
			swapChainDesc.Flags));

		//��ȡ��ǰ�ĺ�̨����������
		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		//����RTV
		UpdateRenderTargetViews();
	}
}

const RenderTarget& SwapChain::GetRenderTarget() const
{
	//����ǰ�ĺ�̨���������ӵ���ȾĿ�����ɫ0�ӿ���
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, m_BackBufferTextures[m_CurrentBackBufferIndex]);

	return m_RenderTarget;
}

UINT SwapChain::Present(const std::shared_ptr<Texture>& _texture /*= nullptr*/)
{
	auto commandList = m_CommandQueue.GetCommandList();
	auto backbuffer = m_BackBufferTextures[m_CurrentBackBufferIndex];

	//�ж��Ƿ���Ҫ�������Ƶ���̨����
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

	//ת����Դ״̬Ϊ����
	commandList->TransitionBarrier(backbuffer, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandQueue.ExecuteCommandList(commandList);

	//������ʾ״̬
	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_TearingSupported && !m_FullScreen && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	//��ȡΧ��ֵ
	m_FenceValues[m_CurrentBackBufferIndex] = m_CommandQueue.Signal();

	//��ȡ��һ����̨��������
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	//�ȴ�Χ��ֵ����
	auto fenceValue = m_FenceValues[m_CurrentBackBufferIndex];
	m_CommandQueue.WaitForFenceValue(fenceValue);

	//�ͷŹ�ʱ��������
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

	//��ȡ�������
	auto commandQueue = m_CommandQueue.GetCommandQueue();
	//��ȡ������
	auto adapter = m_Device.GetAdapter();
	auto dxgiAdapter = adapter->GetDXGIAdapter();

	//����������
	Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;

	ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

	ThrowIfFailed(dxgiFactory.As(&dxgiFactory5));

	//���Է�˺�ѵ�֧��
	bool allowTearing = false;
	if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(bool))))
	{
		m_TearingSupported = (allowTearing == true);
	}

	//���㴰�ڴ�С
	RECT windowRect;
	::GetClientRect(_hwnd, &windowRect);

	m_Width = windowRect.right - windowRect.left;
	m_Height = windowRect.bottom - windowRect.top;

	//��������������
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

	//����������
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory5->CreateSwapChainForHwnd(
		commandQueue.Get(),
		m_HWnd,
		&desc,
		nullptr,
		nullptr, &swapChain1));

	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	//�ر�ALT+ENTERȫ���Ĺ���
	ThrowIfFailed(dxgiFactory5->MakeWindowAssociation(m_HWnd, DXGI_MWA_NO_ALT_ENTER));
	
	//�������֡�ӳ�
	m_SwapChain->SetMaximumFrameLatency(BufferCount - 1);

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	//��ȡ�������ɵȴ�����
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
