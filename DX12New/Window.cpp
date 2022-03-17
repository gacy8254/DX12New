#include "Window.h"

void Window::Destroy()
{
	if (auto pGame = m_pGame.lock())
	{
		//��֪��Ϸ�࣬���ڼ����´ݻ�
		pGame->OnWindowDestroy();
	}
	if (m_HWND)
	{
		DestroyWindow(m_HWND);
		m_HWND = nullptr;
	}
}

void Window::SetFullScreen(bool _fullScreen)
{
	if (m_FullScreen != _fullScreen)
	{
		m_FullScreen = _fullScreen;

		if (m_FullScreen)
		{
			::GetWindowRect(m_HWND, &m_WindowRect);

			UINT windowStyle = WS_OVERLAPPED & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(m_HWND, GWL_STYLE, windowStyle);

			HMONITOR monitor = ::MonitorFromWindow(m_HWND, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX info = {};
			info.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(monitor, &info);

			::SetWindowPos(m_HWND, HWND_TOP,
				info.rcMonitor.left,
				info.rcMonitor.top,
				info.rcMonitor.right - info.rcMonitor.left,
				info.rcMonitor.bottom - info.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_HWND, SW_MAXIMIZE);
		}
		else
		{
			//�ָ�ȫ��ǰ�Ĵ��ڴ�С
			::SetWindowLong(m_HWND, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_HWND, HWND_NOTOPMOST,
				m_WindowRect.left,
				m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_HWND, SW_NORMAL);
		}
	}
}

UINT Window::Present()
{
	//����vsync
	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	//����
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	//���º�̨�����������
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRTV() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
}

Window::Window(HWND _hwnd, std::wstring& _windowName, int _width, int _height, bool _vsync)
	:m_HWND(_hwnd), m_WindowName(_windowName), m_Width(_width), m_Height(_height), m_VSync(_vsync), m_FrameCounter(0)
{
	Application& app = Application::Get();

	m_TearingSupported = app.IsTearingSupported();

	//����������
	m_SwapChain = CreateSwapChain();
	//����RTV��
	m_RTVHeap = app.CreateDescriptorHeap(m_BufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_RTVDescriptorSize = app.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//����RTV
	UpdateRTVs();
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
	Application& app = Application::Get();

	ComPtr<IDXGISwapChain4> swapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;

	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = _width;
	desc.Height = _height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc = { 1, 0 };
	desc.BufferCount = m_BufferCount;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = m_TearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


	ID3D12CommandQueue* pCommandQueue = app.GetCommandQueue()->GetCommandQueue().Get();
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		pCommandQueue,
		_hwnd,
		&desc,
		nullptr,
		nullptr, &swapChain1));

	//�ر�ALT+ENTERȫ���Ĺ���
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_HWND, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain4));

	m_CurrentBackBufferIndex = swapChain4->GetCurrentBackBufferIndex();

	return swapChain4;
}

void Window::UpdateRTVs()
{
	auto device = Application::Get().GetDevice();

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < m_BufferCount; i++)
	{
		ComPtr<ID3D12Resource> backbuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		device->CreateRenderTargetView(backbuffer.Get(), nullptr, handle);
		m_BackBuffers[i] = backbuffer;

		handle.Offset(m_RTVDescriptorSize);
	}
}

