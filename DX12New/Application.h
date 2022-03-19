#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>


class Window;
class Game;
class CommandQueue;

class Application
{
public:
	static void Create(HINSTANCE _hInst);

	static void Destroy();

	static Application& Get();

	//检查是否支持VSync
	bool IsTearingSupported() const { return m_TearingSupported; }

	//创建一个渲染窗口实例
	//_windowName 该名称将出现在窗口的标题栏中，该名称应该是唯一的
	//返回创建的窗口实例
	std::shared_ptr<Window> CreateRenderWindow(const std::wstring& _windowName, int _width, int _height, bool _vSync = true);

	//销毁窗口
	void DestroyWindow(const std::wstring& _windowName);
	void DestroyWindow(std::shared_ptr<Window> _window);
	//通过名字获取窗口实例
	std::shared_ptr<Window> GetWindowByName(const std::wstring& _windowName);

	//运行程序循环和消息处理
	//返回错误代码
	int Run(std::shared_ptr<Game> pGame);

	//请求退出并关闭所有窗口
	void Quit(int _exitCode = 0);

	//获取设备
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const { return m_Device; }

	//获取队列，有效的Type有
	//D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
	//D3D12_COMMAND_LIST_TYPE_COMPUTE : Can be used for dispatch or copy commands.
	//D3D12_COMMAND_LIST_TYPE_COPY : Can be used for copy commands.
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

	//刷新队列
	void Flush();

	//创建 描述符堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT _numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE _type);
	//获取描述符的大小
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE _type) const;

	static uint64_t GetFrameCount();

protected:
	//创建应用程序实例
	Application(HINSTANCE _hIns);

	virtual ~Application();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool _useWarp);
	Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> _adapter);

	//检查是否支持VSync
	bool CheckTearingSupport();

private:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	HINSTANCE m_HIntance;

	Microsoft::WRL::ComPtr<ID3D12Device2> m_Device = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter = nullptr;

	std::shared_ptr<CommandQueue> m_DirectCommandQueue;
	std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
	std::shared_ptr<CommandQueue> m_CopyCommandQueue;

	bool m_TearingSupported = false;

	static uint64_t ms_FrameCount;
};

