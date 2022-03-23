#include "Window.h"

#include "Application.h"
#include "CommandQueue.h"
#include "D3D12LibPCH.h"
#include "Game.h"
#include "ResourceStateTracker.h"
#include "CommandList.h"

void Window::Destroy()
{
	if (auto pGame = m_pGame.lock())
	{
		//��֪��Ϸ�࣬���ڼ����´ݻ�
		//pGame->OnWindowDestroy();
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

UINT Window::Present(const Texture& _texture)
{
	//��ȡ����Ķ���
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	auto& backBuffer = m_BackBufferTextures[m_CurrentBackBufferIndex];

	//�����ͼ��Ч
	//�ж���ͼ�Ƿ��ǳ�������ͼ���ǾͲ������Դ
	//���ǾͿ�����Դ
	if (_texture.IsVaild())
	{
		if (_texture.GetResourceDesc().SampleDesc.Count > 1)
		{
			commandList->ResolveSubresource(backBuffer, _texture);
		}
		else
		{
			commandList->CopyResource(backBuffer, _texture);
		}
	}

	RenderTarget rT;
	rT.AttachTexture(AttachmentPoint::Color0, backBuffer);

	commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
	commandQueue->ExecuteCommandList(commandList);

	//����vsync
	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	//����
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	m_FenceValue[m_CurrentBackBufferIndex] = commandQueue->Signal();
	m_FrameValue[m_CurrentBackBufferIndex] = Application::GetFrameCount();

	//���º�̨�����������
	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	commandQueue->WaitForFenceValue(m_FrameValue[m_CurrentBackBufferIndex]);

	return m_CurrentBackBufferIndex;
}

const RenderTarget& Window::GetRenderTarget() const
{
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, m_BackBufferTextures[m_CurrentBackBufferIndex]);
	return m_RenderTarget;
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRTV() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
}

Window::Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync)
	:m_HWND(_hwnd), m_WindowName(_windowName), m_Width(_width), m_Height(_height), m_VSync(_vsync), m_FrameCounter(0), m_FenceValue{0}, m_FrameValue{0}
{
	Application& app = Application::Get();

	m_TearingSupported = app.IsTearingSupported();

	//����������
	m_SwapChain = CreateSwapChain();
	
	for (int i = 0; i < m_BufferCount; i++)
	{
		m_BackBufferTextures[i].SetName(L"BackBuffer[" + std::to_wstring(i) + L"]");
	}

	//����RTV
	UpdateRTVs();
}

Window::~Window()
{
	assert(!m_HWND && "�����ٴ���ǰ���Ѿ�ʹ��Application�����˴���");
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
	Application& app = Application::Get();

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain4;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;

	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = m_Width;
	desc.Height = m_Height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc = { 1, 0 };
	desc.BufferCount = m_BufferCount;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


	ID3D12CommandQueue* pCommandQueue = app.GetCommandQueue()->GetCommandQueue().Get();
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		pCommandQueue,
		m_HWND,
		&desc,
		nullptr,
		nullptr, &swapChain1));

	//�ر�ALT+ENTERȫ���Ĺ���
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_HWND, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain4));

	m_CurrentBackBufferIndex = swapChain4->GetCurrentBackBufferIndex();

	return swapChain4;
}

void Window::RegisterCallBacks(std::shared_ptr<Game> _pGame)
{
	m_pGame = _pGame;
}

void Window::OnUpdate(UpdateEventArgs& _e)
{
	m_UpdateClock.Tick();

	if (auto pGame = m_pGame.lock())
	{
		m_FrameCounter++;

		UpdateEventArgs updateEvent(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds(), _e.FrameCount);
		pGame->OnUpdate(updateEvent);
	}
}

void Window::OnRender(RenderEventArgs& _e)
{
	m_RenderClock.Tick();

	if (auto pGame = m_pGame.lock())
	{
		m_FrameCounter++;

		RenderEventArgs renderEvent(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds(), _e.FrameCount);
		pGame->OnRender(renderEvent);
	}
}

void Window::OnKeyPressed(KeyEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnKeyPressed(_e);
	}
}

void Window::OnKeyReleased(KeyEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnKeyReleased(_e);
	}
}

void Window::OnMouseMoved(MouseMotionEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnMouseMoved(_e);
	}
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnMouseButtonPressed(_e);
	}
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnMouseButtonReleased(_e);
	}
}

void Window::OnMouseWheel(MouseWheelEventArgs& _e)
{
	if (auto pGame = m_pGame.lock())
	{
		pGame->OnMouseWheel(_e);
	}
}

void Window::OnResize(ResizeEventArgs& _e)
{
	if (m_Width != _e.Width || m_Height != _e.Height)
	{
		m_Width = _e.Width;
		m_Height = _e.Height;

		//ˢ��GPU
		Application::Get().Flush();

		//�ͷ���������
		m_RenderTarget.AttachTexture(AttachmentPoint::Color0, Texture());

		for (int i = 0; i < m_BufferCount; i++)
		{
			ResourceStateTracker::RemoveGlobalResourceState(m_BackBufferTextures[i].GetResource().Get());
			m_BackBufferTextures[i].Reset();
		}

		DXGI_SWAP_CHAIN_DESC desc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&desc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(m_BufferCount, m_Width, m_Height, desc.BufferDesc.Format, desc.Flags));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		UpdateRTVs();
	}

	if (auto pGame = m_pGame.lock())
	{
		pGame->OnResize(_e);
	}
}

void Window::UpdateRTVs()
{
	for (int i = 0; i < m_BufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backbuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		ResourceStateTracker::AddGlobalResourceState(backbuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

		m_BackBufferTextures[i].SetResource(backbuffer);
		m_BackBufferTextures[i].CreateViews();
	}
}

