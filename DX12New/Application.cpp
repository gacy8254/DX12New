

#include "CommandQueue.h"
#include "Window.h"
#include "Game.h"
#include "Application.h"
#include "D3D12LibPCH.h"


constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::map<HWND, WindowPtr>;
using windowNameMap = std::map<std::wstring, WindowPtr>;

static Application* gs_Application = nullptr;
static WindowMap gs_Windows;
static windowNameMap gs_WindowByName;

uint64_t Application::ms_FrameCount = 0;

struct MakeWindow : public Window
{
	MakeWindow(HWND _hWnd, const std::wstring& _windowName, int _width, int _height, bool _vSync)
		: Window(_hWnd, _windowName, _width, _height, _vSync)
	{}
};

//移除一个窗口
static void RemoveWindow(HWND _hWnd)
{
	WindowMap::iterator iter = gs_Windows.find(_hWnd);
	if (iter != gs_Windows.end())
	{
		WindowPtr pWindow = iter->second;
		gs_WindowByName.erase(pWindow->GetWindowName());
		gs_Windows.erase(iter);
	}
}

// 将消息ID转换成鼠标按键ID
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Middel;
	}
	break;
	}

	return mouseButton;
}

static LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

static LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
{
	WindowPtr pWindow;
	WindowMap::iterator iter = gs_Windows.find(_hwnd);
	if (iter != gs_Windows.end())
	{
		pWindow = iter->second;
	}

	if (pWindow)
	{
		switch (_message)
		{
		//绘制
		case WM_PAINT:
		{
			++Application::ms_FrameCount;
			UpdateEventArgs updateEventArgs(0.0f, 0.0f, Application::ms_FrameCount);
			pWindow->OnUpdate(updateEventArgs);
			RenderEventArgs renderEventArgs(0.0f, 0.0f, Application::ms_FrameCount);
			pWindow->OnRender(renderEventArgs);
		}
		break;
		//键盘按键按下
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;

			unsigned int c = 0;

			if (PeekMessage(&charMsg, _hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				GetMessage(&charMsg, _hwnd, 0, 0);
				c = static_cast<unsigned int>(charMsg.wParam);
			}
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)_wParam;
			unsigned int scanCode = (_lParam & 0x00FF0000) >> 16;
			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
			pWindow->OnKeyPressed(keyEventArgs);
		}
		break;
		//键盘按键释放
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)_wParam;
			unsigned int c = 0;
			unsigned int scanCode = (_lParam & 0x00FF0000) >> 16;

			unsigned char keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translateCharacters[4];
			if (int result = ToUnicodeEx(static_cast<UINT>(_wParam), scanCode, keyboardState, translateCharacters, 4, 0, NULL) > 0)
			{
				c = translateCharacters[0];
			}
			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
			pWindow->OnKeyReleased(keyEventArgs);
		}
		case WM_SYSCHAR:
		break;
		//鼠标移动
		case WM_MOUSEMOVE:
		{
			bool leftButton = (_wParam & MK_LBUTTON) != 0;
			bool rightButton = (_wParam & MK_RBUTTON) != 0;
			bool midderButton = (_wParam & MK_MBUTTON) != 0;
			bool shift = (_wParam & MK_SHIFT) != 0;
			bool control = (_wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(_lParam));
			int y = ((int)(short)HIWORD(_lParam));

			MouseMotionEventArgs event(leftButton, midderButton, rightButton, control, shift, x, y);
			pWindow->OnMouseMoved(event);
		}
		break;
		//鼠标按键按下
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			bool lButton = (_wParam & MK_LBUTTON) != 0;
			bool rButton = (_wParam & MK_RBUTTON) != 0;
			bool mButton = (_wParam & MK_MBUTTON) != 0;
			bool shift = (_wParam & MK_SHIFT) != 0;
			bool control = (_wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(_lParam));
			int y = ((int)(short)HIWORD(_lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(_message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;
		//鼠标按键抬起
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			bool lButton = (_wParam & MK_LBUTTON) != 0;
			bool rButton = (_wParam & MK_RBUTTON) != 0;
			bool mButton = (_wParam & MK_MBUTTON) != 0;
			bool shift = (_wParam & MK_SHIFT) != 0;
			bool control = (_wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(_lParam));
			int y = ((int)(short)HIWORD(_lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(_message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonReleased(mouseButtonEventArgs);
		}
		break;
		//鼠标滚轮
		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated.
			// A positive value indicates the wheel was rotated to the right.
			// A negative value indicates the wheel was rotated to the left.
			float zDelta = ((int)(short)HIWORD(_wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(_wParam);

			bool lButton = (keyStates & MK_LBUTTON) != 0;
			bool rButton = (keyStates & MK_RBUTTON) != 0;
			bool mButton = (keyStates & MK_MBUTTON) != 0;
			bool shift = (keyStates & MK_SHIFT) != 0;
			bool control = (keyStates & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(_lParam));
			int y = ((int)(short)HIWORD(_lParam));

			// Convert the screen coordinates to client coordinates.
			POINT clientToScreenPoint;
			clientToScreenPoint.x = x;
			clientToScreenPoint.y = y;
			ScreenToClient(_hwnd, &clientToScreenPoint);

			MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
			pWindow->OnMouseWheel(mouseWheelEventArgs);
		}
		break;
		//改变大小
		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(_lParam));
			int height = ((int)(short)HIWORD(_lParam));

			ResizeEventArgs resizeEventArgs(width, height);
			pWindow->OnResize(resizeEventArgs);
		}
		break;
		//窗口关闭
		case WM_DESTROY:
			RemoveWindow(_hwnd);
			if (gs_Windows.empty())
			{
				PostQuitMessage(0);
			}
			break;
		default:
			return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
		}
	}
	else
	{
		return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
	}
	return 0;
}

void Application::Create(HINSTANCE _hInst)
{
	if (!gs_Application)
	{
		gs_Application = new Application(_hInst);
	}
}

void Application::Destroy()
{
	if (gs_Application)
	{
		assert(gs_Windows.empty() && gs_WindowByName.empty() && "所有的窗口都应在销毁实例之前被摧毁");

		delete gs_Application;
		gs_Application = nullptr;
	}
}

Application& Application::Get()
{
	assert(gs_Application);
	return *gs_Application;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& _windowName, int _width, int _height, bool _vSync /*= true*/)
{
	//查找是否有同名的窗口存在，如果有就直接返回该窗口
	windowNameMap::iterator windowIt = gs_WindowByName.find(_windowName);
	if (windowIt != gs_WindowByName.end())
	{
		return windowIt->second;
	}

	RECT windowRect = { 0, 0, _width, _height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = CreateWindowW(WINDOW_CLASS_NAME, _windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, m_HIntance, nullptr);

	if (!hWnd)
	{
		MessageBoxA(NULL, "无法创建渲染窗口", "错误", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	//创建窗口
	WindowPtr pWindow = std::make_shared<MakeWindow>(hWnd, _windowName, _width, _height, _vSync);
	//将窗口实例加入容器当中
	gs_Windows.insert(WindowMap::value_type(hWnd, pWindow));
	gs_WindowByName.insert(windowNameMap::value_type(_windowName, pWindow));

	return pWindow;
}

void Application::DestroyWindow(const std::wstring& _windowName)
{
	WindowPtr pWindow = GetWindowByName(_windowName);
	if (pWindow)
	{
		DestroyWindow(pWindow);
	}
}

void Application::DestroyWindow(std::shared_ptr<Window> _window)
{
	if (_window)
	{
		_window->Destroy();
	}
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& _windowName)
{
	std::shared_ptr<Window> window;
	windowNameMap::iterator iter = gs_WindowByName.find(_windowName);
	if (iter != gs_WindowByName.end())
	{
		window = iter->second;
	}
	else
	{
		window = nullptr;
	}

	return window;
}

int Application::Run(std::shared_ptr<Game> pGame)
{
	if (!pGame->Init())
	{
		return 1;
	}
	if (!pGame->LoadContent())
	{
		return 2;
	}


	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	Flush();

	pGame->UnLoadContent();
	pGame->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::Quit(int _exitCode /*= 0*/)
{
	PostQuitMessage(_exitCode);
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type /*= D3D12_COMMAND_LIST_TYPE_DIRECT*/) const
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue;
		break;
	default:
		assert(false && "无效的类型");
		break;
	}

	return commandQueue;
}

void Application::Flush()
{
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
	m_DirectCommandQueue->Flush();
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT _numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE _type)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = _numDescriptors;
	desc.Type = _type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	ThrowIfFailed(m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE _type) const
{
	return m_Device->GetDescriptorHandleIncrementSize(_type);
}

uint64_t Application::GetFrameCount()
{
	return ms_FrameCount;
}

Application::Application(HINSTANCE _hIns)
	:m_HIntance(_hIns)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	WNDCLASSEXW windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.hInstance = _hIns;
	windowClass.hIcon = ::LoadIcon(_hIns, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	windowClass.hIconSm = ::LoadIcon(_hIns, NULL);

	if (!RegisterClassExW(&windowClass))
	{
		MessageBoxA(NULL, "注册窗口失败", "错误", MB_OK | MB_ICONERROR);
	}

	//依次创建适配器，设备，三个队列
	m_Adapter = GetAdapter(false);
	//如果没有可用的适配器，则使用软件适配器
	if (!m_Adapter)
	{
		m_Adapter = GetAdapter(true);
	}

	if (m_Adapter)
	{
		m_Device = CreateDevice(m_Adapter);
	}

	if (m_Device)
	{
		m_DirectCommandQueue = std::make_shared<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_CopyCommandQueue = std::make_shared<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_COPY);

		m_TearingSupported = CheckTearingSupport();
	}

	ms_FrameCount = 0;
}

Application::~Application()
{
	Flush();
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter(bool _useWarp)
{
	//创建DXGI工厂
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter4> dxgiAdapter4;
	ComPtr<IDXGIAdapter1> dxgiAdapter1;

	if (_useWarp)
	{
		//如果使用软件适配器则直接找到软件适配器返回
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		//首先枚举所有的显示适配器
		//排除DXGI_ADAPTER_FLAG_SOFTWARE标志的软件适配器
		//找到兼容DX12的硬件显示适配器
		//选择显存最大的一个
		//将IDXGIAdapter1赋值给IDXGIAdapter4
		SIZE_T maxDadicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiadapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiadapterDesc1);

			if ((dxgiadapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiadapterDesc1.DedicatedVideoMemory > maxDadicatedVideoMemory)
			{
				maxDadicatedVideoMemory = dxgiadapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> _adapter)
{
	ComPtr<ID3D12Device2> d3dDevice2;
	ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice2)));

	//开启DEBUG消息
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3dDevice2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		//设置消息过滤
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO                                     //目前没有消息根据类别被忽略
		};

		D3D12_MESSAGE_ID DenyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(severities);
		NewFilter.DenyList.pSeverityList = severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3dDevice2;
}

bool Application::CheckTearingSupport()
{
	bool allowTearing = false;

	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				allowTearing = false;
			}
		}
	}

	return allowTearing == true;
}
