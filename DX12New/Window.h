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



class Game;


class Window
{
public:
	//后台缓冲的数量
	static const UINT m_BufferCount = 3;

	//获取窗口句柄
	HWND GetWindowHandle() { return m_HWND; }

	//摧毁窗口
	void Destroy();

	const std::wstring& GetWindowName()const { return m_WindowName; }

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	//vsync设置函数
	bool IsVSync() const { return m_VSync; }
	void SetVSync(bool _vsync) { m_VSync = _vsync; }
	void ToggleVSync() { SetVSync(!m_VSync); }

	//全屏
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//显示窗口
	void Show() { ::ShowWindow(m_HWND, SW_SHOW); }
	void Hide() { ::ShowWindow(m_HWND, SW_HIDE); }

	//返回当前后台缓冲序号
	UINT GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	//呈现图像，返回当前的缓冲区序号
	UINT Present();

	//获取当前的RTV缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

	//获取后台缓冲区的资源
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	friend class Game;

	friend class Application;

	Window() = delete;
	Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync);
	virtual ~Window();

	//创建交换链
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

	//注册回调函数
	void RegisterCallBacks(std::shared_ptr<Game> _pGame);

	//更新和绘制函数仅仅会被Application调用
	virtual void OnUpdate(UpdateEventArgs& _e);
	virtual void OnRender(RenderEventArgs& _e);

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
	
	//更新RTV
	void UpdateRTVs();

private:
	//禁用拷贝构造
	Window(const Window& copy) = delete;
	Window& operator=(const Window& other) = delete;

	//窗口名字
	std::wstring m_WindowName;
	//窗口句柄
	HWND m_HWND;
	//窗口矩形,用于切换全屏状态时记录窗口尺寸
	RECT m_WindowRect;
	//窗口大小
	int m_Width = 1280;
	int m_Height = 720;

	//用于控制交换链的变量
	bool m_VSync = true;
	bool m_TearingSupported = false;
	//是否全屏
	bool m_FullScreen = false;

	//交换链  RTV堆和后台缓冲区
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[m_BufferCount];

	UINT m_RTVDescriptorSize;//描述符的大小，用于偏移地址
	UINT m_CurrentBackBufferIndex;

	//计时器
	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;
	uint64_t m_FrameCounter;

	std::weak_ptr<Game> m_pGame;
};

