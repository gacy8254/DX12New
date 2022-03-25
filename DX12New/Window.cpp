#include "Window.h"

#include "Application.h"
#include "CommandQueue.h"
#include "D3D12LibPCH.h"
#include "Game.h"
#include "ResourceStateTracker.h"
#include "CommandList.h"

#include <iostream>

void Window::SetWindowTitle(const std::wstring& _title)
{
	m_Title = _title;
	::SetWindowTextW(m_HWND, m_Title.c_str());
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
			//恢复全屏前的窗口大小
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

Window::Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync)
	:m_HWND(_hwnd), 
	m_WindowName(_windowName), 
	m_Width(_width), 
	m_Height(_height), 
	m_Title(_windowName),
	m_PreviousMouseX(0), 
	m_PreviousMouseY(0), 
	m_FullScreen(false),
	m_IsMinimized(false), 
	m_IsMaximized(false),
	m_bInClientRect(false), 
	m_bHasKeyboradFocus(false)
{
	m_DPIscaling = ::GetDpiForWindow(_hwnd) / 96.0f;
}

Window::~Window()
{
	::DestroyWindow(m_HWND);
}

void Window::OnUpdate(UpdateEventArgs& _e)
{
	m_Timer.Tick();

	_e.DeltaTime = m_Timer.ElapsedSeconds();
	_e.TotalTime = m_Timer.TotalSeconds();
	
	Update(_e);
}

void Window::OnClose(WindowCloseEventArgs& _e)
{
	Close(_e);
}

void Window::OnRestored(ResizeEventArgs& _e)
{
	Maximized(_e);
}

void Window::OnMinimized(ResizeEventArgs& _e)
{
	Minimized(_e);
}

void Window::OnMaximized(ResizeEventArgs& _e)
{
	Restored(_e);
}

void Window::OnDPIScaaleChanged(DPIScaleEventArgs& _e)
{
	m_DPIscaling = _e.DPIScale;
	DPIScaleChanged(_e);
}

void Window::OnKetboardFocus(EventArgs& _e)
{
	m_bHasKeyboradFocus = true;
	KeyboardFocus(_e);
}

void Window::OnKetboardBlur(EventArgs& _e)
{
	m_bHasKeyboradFocus = false;
	KeyboardBlur(_e);
}

void Window::OnMouseEnter(MouseMotionEventArgs& _e)
{
	TRACKMOUSEEVENT trackMouseEvent = {};
	trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	trackMouseEvent.hwndTrack = m_HWND;
	trackMouseEvent.dwFlags = TME_LEAVE;
	TrackMouseEvent(&trackMouseEvent);

	m_bInClientRect = true;
	MouseEnter(_e);
}

void Window::OnMouseLeave(EventArgs& _e)
{
	m_bInClientRect = false;
	MouseLeave(_e);
}

void Window::OnMouseFocus(EventArgs& _e)
{
	MouseFocus(_e);
}

void Window::OnMouseBlur(EventArgs& _e)
{
	MouseBlur(_e);
}

void Window::OnKeyPressed(KeyEventArgs& _e)
{
	KeyPressed(_e);
}

void Window::OnKeyReleased(KeyEventArgs& _e)
{
	KeyReleased(_e);
}

void Window::OnMouseMoved(MouseMotionEventArgs& _e)
{
	if (!m_bInClientRect)
	{
		m_PreviousMouseX = _e.X;
		m_PreviousMouseY = _e.Y;
		m_bInClientRect = true;
		//鼠标进入窗口
		OnMouseEnter(_e);
	}

	_e.RelX = _e.X - m_PreviousMouseX;
	_e.RelY = _e.Y - m_PreviousMouseY;

	m_PreviousMouseX = _e.X;
	m_PreviousMouseY = _e.Y;

	MouseMoved(_e);
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{
	MouseButtonPressed(_e);
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{
	MouseButtonReleased(_e);
}

void Window::OnMouseWheel(MouseWheelEventArgs& _e)
{
	MouseWheel(_e);
}

void Window::OnResize(ResizeEventArgs& _e)
{

	m_Width = _e.Width;
	m_Height = _e.Height;

	//非最大最小化
	if ((m_IsMinimized || m_IsMaximized) && _e.State == WindowState::Restored)
	{
		m_IsMaximized = false;
		m_IsMinimized = false;
		OnRestored(_e);
	}
	//最小化
	if (!m_IsMinimized && _e.State == WindowState::Minimized)
	{
		m_IsMinimized = true;
		m_IsMaximized = false;
		OnMinimized(_e);
	}
	//最大化
	if (!m_IsMaximized && _e.State == WindowState::Maximized)
	{
		m_IsMinimized = false;
		m_IsMaximized = true;
		OnMaximized(_e);
	}

	Resize(_e);
}


