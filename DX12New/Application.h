#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <thread>  
#include <memory>
#include <string>
#include <type_traits>
#include <mutex> 
#include <limits> 
#include <cstdint>  

#include <gainput/gainput.h>

#include "DescriptorAllocation.h"
#include "Event.h"

#ifdef CreateWindow
#undef CreateWindow
#endif


class Window;
class Game;
class CommandQueue;
class DescriptorAllocator;

using WndProcEvent = Delegate<LRESULT(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam)>;

class Application
{
public:
	static Application& Create(HINSTANCE _hInst);

	static void Destroy();

	static Application& Get();

	//获取键盘的设备ID
	gainput::DeviceId GetKeyboardId() const { return m_KeyboardDevice; }

	//获取鼠标的设备ID
	gainput::DeviceId GetMouseId() const { return m_MouseDevice; }

	//获取一个输入设备
	template<class T>
	T* GetDevice(gainput::DeviceId _deviceId) const
	{
		static_assert(std::is_base_of_v<gainput::InputDevice, T>);
		return static_cast<T*>(m_InputManager.GetDevice(_deviceId));
	}

	//创建一个输入图
	std::shared_ptr<gainput::InputMap> CreateInputMap(const char* _name = nullptr);

	//开始执行,返回错误代码
	int32_t Run();

	//通知输入管理器,窗口大小改变
	//这是gainput标准化鼠标输入所必须的
	//只在单个窗口刷新大小时调用
	void SetDisplaySize(int _width, int _height);

	//处理输入事件
	void ProcessInput();

	//停止程序
	void Stop();

	//要支持热加载修改过的文件,可以注册一个路径,用于监听文件修改通知
	//文件更改通知通过Application::FileChange设置
	//void RegisterDirectoryChangeListener(const std::wstring& _dir, bool _recursive = true);

	//创建一个渲染窗口实例
	//_windowName 该名称将出现在窗口的标题栏中，该名称应该是唯一的
	//返回创建的窗口实例
	std::shared_ptr<Window> CreateWindow(const std::wstring& _windowName, int _width, int _height);

	//通过名字获取窗口实例
	std::shared_ptr<Window> GetWindowByName(const std::wstring& _windowName) const;

	//当消息发送到窗口时调用
	WndProcEvent WndProcHandler;

	//当文件修改时调用
	FileChangeEvent FileChanged;

	//程序结束时调用
	Event Exit;

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	//创建应用程序实例
	Application(HINSTANCE _hIns);

	virtual ~Application();

	//检测到文件修改
	//virtual void OnFileChange(FileChangedEventArgs& _e);

	//窗口消息处理器
	virtual LRESULT OnWndProc(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam);

	virtual void OnExit(EventArgs& _e);

private:
	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&) = delete;
	Application& operator=(Application&&) = delete;

	//目录更改侦听器线程入口点函数
	//void CheckFileChanges();

	HINSTANCE m_HIntance;

	//输入设备管理
	gainput::InputManager m_InputManager;
	gainput::DeviceId     m_KeyboardDevice;
	gainput::DeviceId     m_MouseDevice;

	//当程序运行时为真
	std::atomic_bool m_bIsRuning;
	//是否应当退出程序
	std::atomic_bool m_RequestQuit;

};

