#include <DX12LibPCH.h>

#include "CommandQueue.h"
#include "Window.h"
#include "Game.h"
#include "Application.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::map<HWND, WindowPtr>;
using windowNameMap = std::map<std::wstring, WindowPtr>;

static Application* gs_Application = nullptr;
static WindowMap gs_Windows;
static windowNameMap gs_WindowByName;

struct MakeWindow : public Window
{
	MakeWindow(HWND _hWnd, const std::wstring& _windowName, int _width, int _height, bool _vSync)
		: Window(_hWnd, _windowName, _width, _height, _vSync)
	{}
};

LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
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
		case WM_PAINT:
			pWindow->Update();
			pWindow->Render();
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000 != 0);

			switch (_wParam)
			{
			case 'V':
				m_VSync = !m_VSync;
				break;
			case VK_ESCAPE:
				::PostQuitMessage(0);
				break;
			case  VK_RETURN:
				if (alt)
				{
			case VK_F11:
				SetFullScreen(!m_FullScreen);
				}
				break;
			}
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			RECT clientRect = {};
			::GetClientRect(m_HWND, &clientRect);

			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;

			Resize(width, height);
		}
		break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
		}
	}
	else
	{
		return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
	}
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
		MessageBoxA(NULL, "无法创建渲染窗口"， "错误", MB_OK | MB_ICONERROR);
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
	windowClass.hInstance = _hInst;
	windowClass.hIcon = ::LoadIcon(_hInst, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	windowClass.hIconSm = ::LoadIcon(_hInst, NULL);

	if (!RegisterClassExW(&windowClass))
	{
		MessageBoxA(NULL, "注册窗口失败", "错误"， MB_OK | MB_ICONERROR);
	}

	//依次创建适配器，设备，三个队列
	m_Adapter = GetAdapter(false);
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
}

Application::!Application()
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
