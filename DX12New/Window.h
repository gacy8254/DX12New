#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <memory>

//#include "d3dx12.h"
#include "Event.h"
#include "HighResolutionClock.h"
#include "Texture.h"
#include "RenderTarget.h"



class Game;


class Window
{
public:
	//后台缓冲的数量
	static const UINT m_BufferCount = 3;

	//获取窗口句柄
	HWND GetWindowHandle() { return m_HWND; }

	float GetDPIScaling() const { return m_DPIscaling; }

	const std::wstring& GetWindowName()const { return m_WindowName; }

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	//设置窗口标题
	void SetWindowTitle(const std::wstring& _title);
	const std::wstring& GetWindowTitle() const { return m_Title; }

	//全屏
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//显示窗口
	void Show() { ::ShowWindow(m_HWND, SW_SHOW); }
	void Hide() { ::ShowWindow(m_HWND, SW_HIDE); }

	//窗口需要更新时被调用
	UpdateEvent Update;

	//DPI改变时调用
	DPIScaleEvent DPIScaleChanged;

	//窗口关闭时调用
	WindowCloseEvent Close;

	//改变窗口大小时调用
	ResizeEvent Resize;

	//最小化时调用
	ResizeEvent Minimized;

	//最大化时调用
	ResizeEvent Maximized;

	//窗口恢复时调用
	ResizeEvent Restored;

	//键盘按键按下时调用
	KeyboardEvent KeyPressed;

	//键盘按键松开时调用
	KeyboardEvent KeyReleased;

	//获得键盘焦点时调用
	Event KeyboardFocus;

	//丢失键盘时调用
	Event KeyboardBlur;

	//鼠标移动时调用
	MouseMotionEvent MouseMoved;

	//鼠标进入窗口时调用
	MouseMotionEvent MouseEnter;

	//鼠标按下时调用
	MouseButtonEvent MouseButtonPressed;

	//鼠标按键松起时调用
	MouseButtonEvent MouseButtonReleased;

	//滚轮转动时调用
	MouseWheelEvent MouseWheel;

	//鼠标离开窗口时调用
	Event MouseLeave;

	//获得鼠标焦点时调用
	Event MouseFocus;

	//丢失鼠标时调用
	Event MouseBlur;

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	friend class Game;

	friend class Application;

	Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync);
	virtual ~Window();

	//更新和绘制函数仅仅会被Application调用
	virtual void OnUpdate(UpdateEventArgs& _e);

	//窗口关闭
	virtual void OnClose(WindowCloseEventArgs& _e);

	//恢复窗口
	virtual void OnRestored(ResizeEventArgs& _e);

	//最小化
	virtual void OnMinimized(ResizeEventArgs& _e);

	//最大化
	virtual void OnMaximized(ResizeEventArgs& _e);

	//DPI变化
	virtual void OnDPIScaaleChanged(DPIScaleEventArgs& _e);

	//键盘聚焦在窗口
	virtual void OnKetboardFocus(EventArgs& _e);
	//丢失键盘
	virtual void OnKetboardBlur(EventArgs& _e);

	//鼠标进入窗口
	virtual void OnMouseEnter(MouseMotionEventArgs& _e);
	//鼠标离开
	virtual void OnMouseLeave(EventArgs& _e);

	//窗口接受的鼠标信息
	virtual void OnMouseFocus(EventArgs& _e);
	//窗口丢失鼠标
	virtual void OnMouseBlur(EventArgs& _e);

	//键盘按键
	virtual void OnKeyPressed(KeyEventArgs& _e);
	virtual void OnKeyReleased(KeyEventArgs& _e);

	//鼠标事件
	virtual void OnMouseMoved(MouseMotionEventArgs& _e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e);
	virtual void OnMouseWheel(MouseWheelEventArgs& _e);

	//窗口大小调整
	virtual void OnResize(ResizeEventArgs& _e);
	
private:
	//窗口名字
	std::wstring m_WindowName;
	std::wstring m_Title;
	//窗口句柄
	HWND m_HWND;
	//窗口矩形,用于切换全屏状态时记录窗口尺寸
	RECT m_WindowRect;
	//鼠标是否位于窗口中
	bool m_bInClientRect;

	//窗口接收到键盘焦点
	bool m_bHasKeyboradFocus;

	//窗口大小
	int m_Width = 1280;
	int m_Height = 720;

	//窗口的DPI缩放
	float m_DPIscaling;

	//是否最小化或最大化
	bool m_IsMinimized;
	bool m_IsMaximized;

	//是否全屏
	bool m_FullScreen;

	HighResolutionTimer m_Timer;

	int m_PreviousMouseX;
	int m_PreviousMouseY;
};

