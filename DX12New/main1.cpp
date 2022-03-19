//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <shellapi.h> // 为了使用CommandLineToArgW()函数
//
//// The min/max macros conflict with like-named member functions.
//// Only use std::min and std::max defined in <algorithm>.
////下面三个语句的功能差不多，如果定义了这个宏，就取消这个宏的定义
//#if defined(min)
//#undef min
//#endif
//
//#if defined(max)
//#undef max
//#endif
////这个宏定义在Windows.h头文件中
//#if defined(CreateWindow)
//#undef CreateWindow
//#endif
//
//// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
////Com组件需要的头文件
//#include <wrl.h>
//using namespace Microsoft::WRL;
//
////DX12头文件
//#include <d3d12.h>
//#include <dxgi1_6.h>
//#include <d3dcompiler.h>
//#include <DirectXMath.h>
//#include "d3dx12.h"
//
////标准库
//#include <algorithm>
//#include <cassert>
//#include <chrono>
//#include <string>
//#include <conio.h>
//
////自己的文件
//#include "helpers.h"
//
////后台缓冲的数量
//const uint8_t m_NumFrames = 3;
////是否使用wrap适配器
//bool m_UseWrap = false;
//
//uint32_t m_Width = 1280;
//uint32_t m_Height = 720;
//
////是否已经初始化,初始化工作完成后将被设置为true
//bool m_Initialized = false;
//
////窗口句柄
//HWND m_HWND;
////窗口矩形,用于切换全屏状态时记录窗口尺寸
//RECT m_WindowRect;
//
////Dx12对象
//ComPtr<ID3D12Device2> m_Device = nullptr;
//ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
//ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;
//ComPtr<ID3D12Resource> m_BackBuffers[m_NumFrames];
//ComPtr<ID3D12GraphicsCommandList> m_CommandList = nullptr;
//ComPtr<ID3D12CommandAllocator> m_CommandAllocator[m_NumFrames];
//ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
//UINT m_RTVDescriptorSize;//描述符的大小，用于偏移地址
//UINT m_CurrentBackBufferIndex;
//
////用于同步的变量
//ComPtr<ID3D12Fence> m_Fence = nullptr;
//uint64_t m_FenceValue = 0;
//uint64_t m_FrameFenceValues[m_NumFrames] = {};
//HANDLE m_FenceEvent;
//
////用于控制交换链的变量
//bool m_VSync = true;
//bool m_TearingSupported = false;
////是否全屏
//bool m_FullScreen = false;
//
////回调函数
////LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//
////命令行参数解析，允许使用命令行参数覆盖一些全局变量的值
//void ParseCommandLineArguments()
//{
//	int argc;
//	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
//
//	for (size_t i = 0; i < argc; i++)
//	{
//		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
//		{
//			m_Width = ::wcstol(argv[++i], nullptr, 10);
//		}
//		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
//		{
//			m_Height = ::wcstol(argv[++i], nullptr, 10);
//		}
//		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
//		{
//			m_UseWrap = true;
//		}
//	}
//
//	::LocalFree(argv);
//}
//
////开启Debug层
//void EnableDebugLayer()
//{
//#if defined(_DEBUG)
//	ComPtr<ID3D12Debug> debugInterface;
//	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
//	debugInterface->EnableDebugLayer();
//#endif
//}
//
////注册窗口类
//void RegisterWindowClass(HINSTANCE _hInst, const wchar_t* _windowClassName)
//{
//	//WNDCLASSEXW windowClass = {};
//	//windowClass.cbSize = sizeof(WNDCLASSEXW);
//	//windowClass.style = CS_HREDRAW | CS_VREDRAW;
//	//windowClass.lpfnWndProc = &WndProc;
//	//windowClass.cbClsExtra = 0;
//	//windowClass.cbWndExtra = 0;
//	//windowClass.hInstance = _hInst;
//	//windowClass.hIcon = ::LoadIcon(_hInst, NULL);
//	//windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
//	//windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//	//windowClass.lpszMenuName = NULL;
//	//windowClass.lpszClassName = _windowClassName;
//	//windowClass.hIconSm = ::LoadIcon(_hInst, NULL);
//
//	//static ATOM atom = ::RegisterClassExW(&windowClass);
//	//assert(atom > 0);
//}
//
////创建窗口
//HWND CreateWindow(const wchar_t* _windowClassName, HINSTANCE _hInst, const wchar_t* _windowTitle, uint32_t _width, uint32_t _height)
//{
//	//检索显示器的宽和高
//	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
//	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
//
//
//	RECT WindowRect = { 0, 0, static_cast<long>(_width), static_cast<long>(_height) };
//	::AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, false);
//
//	int windowWidth = WindowRect.right - WindowRect.left;
//	int windowHeight = WindowRect.bottom - WindowRect.top;
//
//	//将窗口呈现在屏幕的中心
//	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
//	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);
//
//	//创建窗口
//	HWND hWnd = ::CreateWindowExW(
//		NULL,
//		_windowClassName,
//		_windowTitle,
//		WS_OVERLAPPEDWINDOW,
//		windowX,
//		windowY,
//		windowWidth,
//		windowHeight,
//		NULL,
//		NULL,
//		_hInst,
//		nullptr);
//
//	assert(hWnd && "创建窗口失败");
//
//	return hWnd;
//}
//
////查询显示适配器
//ComPtr<IDXGIAdapter4> GetAdapter(bool _useWarp)
//{
//	//创建DXGI工厂
//	ComPtr<IDXGIFactory4> dxgiFactory;
//	UINT createFactoryFlags = 0;
//
//#if defined(_DEBUG)
//	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
//#endif
//
//	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
//
//	ComPtr<IDXGIAdapter4> dxgiAdapter4;
//	ComPtr<IDXGIAdapter1> dxgiAdapter1;
//
//	if (_useWarp)
//	{
//		//如果使用软件适配器则直接找到软件适配器返回
//		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
//		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
//	}
//	else
//	{
//		//首先枚举所有的显示适配器
//		//排除DXGI_ADAPTER_FLAG_SOFTWARE标志的软件适配器
//		//找到兼容DX12的硬件显示适配器
//		//选择显存最大的一个
//		//将IDXGIAdapter1赋值给IDXGIAdapter4
//		SIZE_T maxDadicatedVideoMemory = 0;
//		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
//		{
//			DXGI_ADAPTER_DESC1 dxgiadapterDesc1;
//			dxgiAdapter1->GetDesc1(&dxgiadapterDesc1);
//
//			if ((dxgiadapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && 
//				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
//				dxgiadapterDesc1.DedicatedVideoMemory > maxDadicatedVideoMemory)
//			{
//				maxDadicatedVideoMemory = dxgiadapterDesc1.DedicatedVideoMemory;
//				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
//			}
//		}
//	}
//
//	return dxgiAdapter4;
//}
//
////创建设备
//ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> _adapter)
//{
//	ComPtr<ID3D12Device2> d3dDevice2;
//	ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice2)));
//
//	//开启DEBUG消息
//#if defined(_DEBUG)
//	ComPtr<ID3D12InfoQueue> pInfoQueue;
//	if (SUCCEEDED(d3dDevice2.As(&pInfoQueue)))
//	{
//		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
//		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
//		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
//
//		//设置消息过滤
//		D3D12_MESSAGE_SEVERITY severities[] = 
//		{
//			D3D12_MESSAGE_SEVERITY_INFO                                     //目前没有消息根据类别被忽略
//		};
//
//		D3D12_MESSAGE_ID DenyIds[] =
//		{
//			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
//			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
//			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
//		};
//
//		D3D12_INFO_QUEUE_FILTER NewFilter = {};
//		NewFilter.DenyList.NumSeverities = _countof(severities);
//		NewFilter.DenyList.pSeverityList = severities;
//		NewFilter.DenyList.NumIDs = _countof(DenyIds);
//		NewFilter.DenyList.pIDList = DenyIds;
//
//		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
//	}
//#endif
//
//	return d3dDevice2;
//}
//
////创建队列
//ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> _device, D3D12_COMMAND_LIST_TYPE _type)
//{
//	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
//
//	D3D12_COMMAND_QUEUE_DESC desc = {};
//	desc.Type = _type;
//	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//	desc.NodeMask = 0;
//
//	ThrowIfFailed(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
//
//	return d3d12CommandQueue;
//}
//
////检查对于VSync的支持
//bool CheckTearingSupport()
//{
//	bool allowTearing = false;
//
//	ComPtr<IDXGIFactory4> factory4;
//	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
//	{
//		ComPtr<IDXGIFactory5> factory5;
//		if (SUCCEEDED(factory4.As(&factory5)))
//		{
//			if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
//			{
//				allowTearing = false;
//			}
//		}
//	}
//
//	return allowTearing == true;
//}
//
////创建交换链
//ComPtr<IDXGISwapChain4> CreateSwapChain(HWND _hwnd, ComPtr<ID3D12CommandQueue> _commandQueue, uint32_t _width, uint32_t _height, uint32_t _bufferCount)
//{
//	ComPtr<IDXGISwapChain4> swapChain4;
//	ComPtr<IDXGIFactory4> dxgiFactory4;
//
//	UINT createFactoryFlags = 0;
//#if defined(_DEBUG)
//	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
//#endif
//
//	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
//
//	DXGI_SWAP_CHAIN_DESC1 desc = {};
//	desc.Width = _width;
//	desc.Height = _height;
//	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	desc.Stereo = FALSE;
//	desc.SampleDesc = { 1, 0 };
//	desc.BufferCount = _bufferCount;
//	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	desc.Scaling = DXGI_SCALING_STRETCH;
//	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//	desc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
//	
//	ComPtr<IDXGISwapChain1> swapChain1;
//	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(_commandQueue.Get(),
//		_hwnd,
//		&desc,
//		nullptr,
//		nullptr, &swapChain1));
//
//	//关闭ALT+ENTER全屏的功能
//	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER));
//	ThrowIfFailed(swapChain1.As(&swapChain4));
//
//	return swapChain4;
//}
//
////创建描述符堆
//ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptors)
//{
//	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
//
//	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
//	desc.NumDescriptors = _numDescriptors;
//	desc.Type = _type;
//
//	ThrowIfFailed(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
//
//	return descriptorHeap;
//}
//
////创建RTV
//void UpdateRenderTargetView(ComPtr<ID3D12Device2> _device, ComPtr<IDXGISwapChain4> _swapChain, ComPtr<ID3D12DescriptorHeap> _heap)
//{
//	//获取描述符的大小
//	auto rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_heap->GetCPUDescriptorHandleForHeapStart());
//
//	for (int i = 0; i < m_NumFrames; i++)
//	{
//		ComPtr<ID3D12Resource> backbuffer;
//		ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
//
//		_device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtvHandle);
//		m_BackBuffers[i] = backbuffer;
//
//		rtvHandle.Offset(rtvDescriptorSize);
//	}
//}
//
////创建命令分配器
//ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> _device, D3D12_COMMAND_LIST_TYPE _type)
//{
//	ComPtr<ID3D12CommandAllocator> commandAllocator;
//	ThrowIfFailed(_device->CreateCommandAllocator(_type, IID_PPV_ARGS(&commandAllocator)));
//
//	return commandAllocator;
//}
//
////创建命令列表
//ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> _device, ComPtr<ID3D12CommandAllocator> _commandAllocator, D3D12_COMMAND_LIST_TYPE _type)
//{
//	ComPtr<ID3D12GraphicsCommandList> commandList;
//	ThrowIfFailed(_device->CreateCommandList(0, _type, _commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
//
//	//命令列表在使用前必须是关闭状态
//	ThrowIfFailed(commandList->Close());
//
//	return commandList;
//}
//
////创建围栏
//ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> _device)
//{
//	ComPtr<ID3D12Fence> fence;
//
//	ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
//
//	return fence;
//}
//
////创建围栏事件句柄
//HANDLE CreateEventHandle()
//{
//	HANDLE fenceEvent;
//
//	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
//	assert(fenceEvent && "创建围栏事件失败");
//
//	return fenceEvent;
//}
//
////GPU信号
//uint64_t Signal(ComPtr<ID3D12CommandQueue> _commandQueue, ComPtr<ID3D12Fence> _fence, uint64_t& _fenceValue)
//{
//
//	uint64_t fenceValueForSignal = ++_fenceValue;
//	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fenceValueForSignal));
//
//	return fenceValueForSignal;
//}
//
////等待围栏值到达
//void WaitForFenceValue(ComPtr<ID3D12Fence> _fence, uint64_t _fenceValue, HANDLE _fenceEvent,
//	std::chrono::milliseconds duration = std::chrono::milliseconds::max())
//{
//	if (_fence->GetCompletedValue() < _fenceValue)
//	{
//		ThrowIfFailed(_fence->SetEventOnCompletion(_fenceValue, _fenceEvent));
//		::WaitForSingleObject(_fenceEvent, static_cast<DWORD>(duration.count()));
//	}
//}
//
////flush
//void Flush(ComPtr<ID3D12CommandQueue> _commandQueue, ComPtr<ID3D12Fence> _fence, uint64_t& _fenceValue, HANDLE _fenceEvent)
//{
//	//获取GPU中的围栏值
//	uint64_t fenceValueForSignal = Signal(_commandQueue, _fence, _fenceValue);
//	//等待围栏值到达
//	WaitForFenceValue(_fence, fenceValueForSignal, _fenceEvent);
//}
//
////更新函数
//void Update()
//{
//	static uint64_t frameCounter = 0;
//	static double elasedSeconds = 0.0f;
//	static std::chrono::high_resolution_clock clock;
//	static auto t0 = clock.now();
//
//	frameCounter++;
//	auto t1 = clock.now();
//	auto deltaTime = t1 - t0;
//	t0 = t1;
//
//
//	elasedSeconds += deltaTime.count() * 1e-9;
//	if (elasedSeconds > 1.0f)
//	{
//		char buffer[500];
//		auto fps = frameCounter / elasedSeconds;
//		sprintf_s(buffer, 500, "FPS: %f\n", fps);
//		WCHAR wszClassName[256];
//		memset(wszClassName, 0, sizeof(wszClassName));
//		MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer) + 1, wszClassName,
//			sizeof(wszClassName) / sizeof(wszClassName[0]));
//		OutputDebugString(wszClassName);
//
//		frameCounter = 0;
//		elasedSeconds = 0.0f;
//	}
//}
//
////渲染
//void Render()
//{
//	//获取当前的命令分配器和后台缓冲
//	auto commandAllocator = m_CommandAllocator[m_CurrentBackBufferIndex];
//	auto backbuffer = m_BackBuffers[m_CurrentBackBufferIndex];
//
//	//重置分配器和列表
//	commandAllocator->Reset();
//	m_CommandList->Reset(commandAllocator.Get(), nullptr);
//
//	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backbuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
//	m_CommandList->ResourceBarrier(1, &barrier);
//
//	//清空缓冲区
//	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
//	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
//
//	m_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
//
//	//转换资源状态
//	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backbuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
//	m_CommandList->ResourceBarrier(1, &barrier);
//
//	//关闭列表
//	ThrowIfFailed(m_CommandList->Close());
//
//	//向队列中添加命令
//	ID3D12CommandList* const commandlist[] = { m_CommandList.Get() };
//	m_CommandQueue->ExecuteCommandLists(_countof(commandlist), commandlist);
//
//	//获取当前GPU的围栏值
//	m_FrameFenceValues[m_CurrentBackBufferIndex] = Signal(m_CommandQueue, m_Fence, m_FenceValue);
//	
//	//设置vsync
//	UINT syncInterval = m_VSync ? 1 : 0;
//	UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
//	//呈现
//	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));
//
//	//更新后台缓冲区的序号
//	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
//	//等待
//	WaitForFenceValue(m_Fence, m_FrameFenceValues[m_CurrentBackBufferIndex], m_FenceEvent);
//}
//
////调整大小
//void Resize(uint32_t _width, uint32_t _height)
//{
//	if (m_Width != _width || m_Height != _height)
//	{
//		m_Width = _width;
//		m_Height = _height;
//
//		//刷新GPU
//		Flush(m_CommandQueue, m_Fence, m_FenceValue, m_FenceEvent);
//		
//		for (int i = 0; i < m_NumFrames; i++)
//		{
//			m_BackBuffers[i].Reset();
//			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
//		}
//
//		DXGI_SWAP_CHAIN_DESC desc = {};
//		ThrowIfFailed(m_SwapChain->GetDesc(&desc));
//		ThrowIfFailed(m_SwapChain->ResizeBuffers(m_NumFrames, m_Width, m_Height, desc.BufferDesc.Format, desc.Flags));
//
//		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
//
//		UpdateRenderTargetView(m_Device, m_SwapChain, m_RTVHeap);
//	}
//
//	
//}
//
//void SetFullScreen(bool _fullScreen)
//{
//	if (m_FullScreen != _fullScreen)
//	{
//		m_FullScreen = _fullScreen;
//
//		if (m_FullScreen)
//		{
//			::GetWindowRect(m_HWND, &m_WindowRect);
//
//			UINT windowStyle = WS_OVERLAPPED & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
//
//			::SetWindowLongW(m_HWND, GWL_STYLE, windowStyle);
//
//			HMONITOR monitor = ::MonitorFromWindow(m_HWND, MONITOR_DEFAULTTONEAREST);
//			MONITORINFOEX info = {};
//			info.cbSize = sizeof(MONITORINFOEX);
//			::GetMonitorInfo(monitor, &info);
//
//			::SetWindowPos(m_HWND, HWND_TOP,
//				info.rcMonitor.left,
//				info.rcMonitor.top,
//				info.rcMonitor.right - info.rcMonitor.left,
//				info.rcMonitor.bottom - info.rcMonitor.top,
//				SWP_FRAMECHANGED | SWP_NOACTIVATE);
//
//			::ShowWindow(m_HWND, SW_MAXIMIZE);
//		}
//		else
//		{
//			//恢复全屏前的窗口大小
//			::SetWindowLong(m_HWND, GWL_STYLE, WS_OVERLAPPEDWINDOW);
//
//			::SetWindowPos(m_HWND, HWND_NOTOPMOST,
//				m_WindowRect.left,
//				m_WindowRect.top,
//				m_WindowRect.right - m_WindowRect.left,
//				m_WindowRect.bottom - m_WindowRect.top,
//				SWP_FRAMECHANGED | SWP_NOACTIVATE);
//
//			::ShowWindow(m_HWND, SW_NORMAL);
//		}
//	}
//}
//
////LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
////{
////	if (m_Initialized)
////	{
////		switch (_message)
////		{
////		case WM_PAINT:
////			Update();
////			Render();
////			break;
////		case WM_SYSKEYDOWN:
////		case WM_KEYDOWN:
////		{
////			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000 != 0);
////
////			switch (_wParam)
////			{
////			case 'V':
////				m_VSync = !m_VSync;
////				break;
////			case VK_ESCAPE:
////				::PostQuitMessage(0);
////				break;
////			case  VK_RETURN:
////				if (alt)
////				{
////			case VK_F11:
////				SetFullScreen(!m_FullScreen);
////				}
////				break;
////			}
////		}
////		break;
////		case WM_SYSCHAR:
////			break;
////		case WM_SIZE:
////		{
////			RECT clientRect = {};
////			::GetClientRect(m_HWND, &clientRect);
////
////			int width = clientRect.right - clientRect.left;
////			int height = clientRect.bottom - clientRect.top;
////
////			Resize(width, height);
////		}
////		break;
////		case WM_DESTROY:
////			::PostQuitMessage(0);
////			break;
////		default:
////			return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
////		}
////	}
////	else
////	{
////		return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
////	}
////}
//
//int CALLBACK wWinMain2(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
//{
//	AllocConsole();
//
//	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//
//	const wchar_t* windowClassName = L"Dx12";
//	ParseCommandLineArguments();
//	EnableDebugLayer();
//
//	m_TearingSupported = CheckTearingSupport();
//
//	RegisterWindowClass(hInstance, windowClassName);
//	m_HWND = CreateWindow(windowClassName, hInstance, L"Learning DX12", m_Width, m_Height);
//
//	::GetWindowRect(m_HWND, &m_WindowRect);
//
//	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(m_UseWrap);
//
//	m_Device = CreateDevice(dxgiAdapter4);
//
//	m_CommandQueue = CreateCommandQueue(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
//
//	m_SwapChain = CreateSwapChain(m_HWND, m_CommandQueue, m_Width, m_Height, m_NumFrames);
//
//	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
//
//	m_RTVHeap = CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_NumFrames);
//	m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//
//	UpdateRenderTargetView(m_Device, m_SwapChain, m_RTVHeap);
//
//	for (int i = 0; i < m_NumFrames; i++)
//	{
//		m_CommandAllocator[i] = CreateCommandAllocator(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
//	}
//
//	m_CommandList = CreateCommandList(m_Device, m_CommandAllocator[m_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
//
//	m_Fence = CreateFence(m_Device);
//	m_FenceEvent = CreateEventHandle();
//
//	m_Initialized = true;
//
//	::ShowWindow(m_HWND, SW_SHOW);
//
//	MSG msg = {};
//	while (msg.message != WM_QUIT)
//	{
//		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//		{
//			::TranslateMessage(&msg);
//			::DispatchMessage(&msg);
//		}
//	}
//
//	Flush(m_CommandQueue, m_Fence, m_FenceValue, m_FenceEvent);
//
//	::CloseHandle(m_FenceEvent);
//	FreeConsole();
//	return 0;
//
//}
//
