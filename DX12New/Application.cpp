
#include "DescriptorAllocator.h"
#include "CommandQueue.h"
#include "Window.h"
#include "Game.h"
#include "Application.h"
#include "D3D12LibPCH.h"

#include <iostream>
#include <fcntl.h>
#include <corecrt_io.h>

constexpr int MAX_CONSOLE_LINES = 500;
static Application* gs_Application = nullptr;
constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

static LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);


using WindowMap = std::map<HWND, std::weak_ptr<Window>>;
using windowNameMap = std::map<std::wstring, std::weak_ptr<Window>>;

static WindowMap gs_WindowMap;
static windowNameMap gs_WindowByName;

static std::mutex gs_WindowHandlesMutex;

struct MakeWindow : public Window
{
	MakeWindow(HWND _hWnd, const std::wstring& _windowName, int _width, int _height)
		: Window(_hWnd, _windowName, _width, _height)
	{}
};
//开启控制台
static void CreateConsole()
{
	// Allocate a console.
	if (AllocConsole())
	{
		HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		// Increase screen buffer to allow more lines of text than the default.
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		GetConsoleScreenBufferInfo(lStdHandle, &consoleInfo);
		consoleInfo.dwSize.Y = MAX_CONSOLE_LINES;
		SetConsoleScreenBufferSize(lStdHandle, consoleInfo.dwSize);
		SetConsoleCursorPosition(lStdHandle, { 0, 0 });

		// Redirect unbuffered STDOUT to the console.
		int   hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
		FILE* fp = _fdopen(hConHandle, "w");
		freopen_s(&fp, "CONOUT$", "w", stdout);
		setvbuf(stdout, nullptr, _IONBF, 0);

		// Redirect unbuffered STDIN to the console.
		lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
		hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
		fp = _fdopen(hConHandle, "r");
		freopen_s(&fp, "CONIN$", "r", stdin);
		setvbuf(stdin, nullptr, _IONBF, 0);

		// Redirect unbuffered STDERR to the console.
		lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
		hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
		fp = _fdopen(hConHandle, "w");
		freopen_s(&fp, "CONOUT$", "w", stderr);
		setvbuf(stderr, nullptr, _IONBF, 0);

		// Clear the error state for each of the C++ standard stream objects. We
		// need to do this, as attempts to access the standard streams before
		// they refer to a valid target will cause the iostream objects to enter
		// an error state. In versions of Visual Studio after 2005, this seems
		// to always occur during startup regardless of whether anything has
		// been read from or written to the console or not.
		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();
	}
}

// 将消息ID转换成鼠标按键ID
static MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButton mouseButton;// = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButton::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButton::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButton::Middle;
	}
	break;
	}

	return mouseButton;
}

//将消息ID转换为按键状态
static ButtonState DecodeButtonState(UINT messageID)
{
	ButtonState buttonState = ButtonState::Pressed;

	switch (messageID)
	{
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		buttonState = ButtonState::Released;
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		buttonState = ButtonState::Pressed;
		break;
	}

	return buttonState;
}

//将消息ID转换为窗口状态
static WindowState DecodeWindowState(WPARAM wParam)
{
	WindowState windowState = WindowState::Restored;

	switch (wParam)
	{
	case SIZE_RESTORED:
		windowState = WindowState::Restored;
		break;
	case SIZE_MINIMIZED:
		windowState = WindowState::Minimized;
		break;
	case SIZE_MAXIMIZED:
		windowState = WindowState::Maximized;
		break;
	default:
		break;
	}

	return windowState;
}

static LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
{

	// Allow for external handling of window messages.
	if (Application::Get().OnWndProc(_hwnd, _message, _wParam, _lParam))
	{
		return 1;
	}

	std::shared_ptr<Window> pWindow;
	{
		auto iter = gs_WindowMap.find(_hwnd);
		if (iter != gs_WindowMap.end())
		{
			pWindow = iter->second.lock();
		}
	}

	if (pWindow)
	{
		switch (_message)
		{
		case WM_DPICHANGED:
		{
			float             dpiScaling = HIWORD(_wParam) / 96.0f;
			DPIScaleEventArgs dpiScaleEventArgs(dpiScaling);
			pWindow->OnDPIScaaleChanged(dpiScaleEventArgs);
		}
		break;
		case WM_PAINT:
		{
			// Delta and total time will be filled in by the Window.
			UpdateEventArgs updateEventArgs(0.0, 0.0);
			pWindow->OnUpdate(updateEventArgs);
		}
		break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;

			// Get the Unicode character (UTF-16)
			unsigned int c = 0;
			// For printable characters, the next message will be WM_CHAR.
			// This message contains the character code we need to send the
			// KeyPressed event. Inspired by the SDL 1.2 implementation.
			if (PeekMessage(&charMsg, _hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				//                GetMessage( &charMsg, hwnd, 0, 0 );
				c = static_cast<unsigned int>(charMsg.wParam);
			}

			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

			KeyCode      key = (KeyCode)_wParam;
			KeyEventArgs keyEventArgs(key, c, KeyState::Pressed, control, shift, alt);
			pWindow->OnKeyPressed(keyEventArgs);
		}
		break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

			KeyCode      key = (KeyCode)_wParam;
			unsigned int c = 0;
			unsigned int scanCode = (_lParam & 0x00FF0000) >> 16;

			// Determine which key was released by converting the key code and
			// the scan code to a printable character (if possible). Inspired by
			// the SDL 1.2 implementation.
			unsigned char keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];
			if (int result =
				ToUnicodeEx((UINT)_wParam, scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
			{
				c = translatedCharacters[0];
			}

			KeyEventArgs keyEventArgs(key, c, KeyState::Released, control, shift, alt);
			pWindow->OnKeyReleased(keyEventArgs);
		}
		break;
		// The default window procedure will play a system notification sound
		// when pressing the Alt+Enter keyboard combination if this message is
		// not handled.
		case WM_SYSCHAR:
			break;
		case WM_KILLFOCUS:
		{
			// Window lost keyboard focus.
			EventArgs eventArgs;
			pWindow->OnKeyboardBlur(eventArgs);
		}
		break;
		case WM_SETFOCUS:
		{
			EventArgs eventArgs;
			pWindow->OnKeyboardFocus(eventArgs);
		}
		break;
		case WM_MOUSEMOVE:
		{
			bool lButton = (_wParam & MK_LBUTTON) != 0;
			bool rButton = (_wParam & MK_RBUTTON) != 0;
			bool mButton = (_wParam & MK_MBUTTON) != 0;
			bool shift = (_wParam & MK_SHIFT) != 0;
			bool control = (_wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(_lParam));
			int y = ((int)(short)HIWORD(_lParam));

			MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseMoved(mouseMotionEventArgs);
		}
		break;
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

			// Capture mouse movement until the button is released.
			SetCapture(_hwnd);

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(_message), ButtonState::Pressed, lButton,
				mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;
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

			// Stop capturing the mouse.
			ReleaseCapture();

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(_message), ButtonState::Released, lButton,
				mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonReleased(mouseButtonEventArgs);
		}
		break;
		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated.
			// A positive value indicates the wheel was rotated forwards (away
			//  the user). A negative value indicates the wheel was rotated
			//  backwards (toward the user).
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
			POINT screenToClientPoint;
			screenToClientPoint.x = x;
			screenToClientPoint.y = y;
			::ScreenToClient(_hwnd, &screenToClientPoint);

			MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift,
				(int)screenToClientPoint.x, (int)screenToClientPoint.y);
			pWindow->OnMouseWheel(mouseWheelEventArgs);
		}
		break;
		// NOTE: Not really sure if these next set of messages are working
		// correctly. Not really sure HOW to get them to work correctly.
		// TODO: Try to fix these if I need them ;)
		case WM_CAPTURECHANGED:
		{
			EventArgs mouseBlurEventArgs;
			pWindow->OnMouseBlur(mouseBlurEventArgs);
		}
		break;
		case WM_MOUSEACTIVATE:
		{
			EventArgs mouseFocusEventArgs;
			pWindow->OnMouseFocus(mouseFocusEventArgs);
		}
		break;
		case WM_MOUSELEAVE:
		{
			EventArgs mouseLeaveEventArgs;
			pWindow->OnMouseLeave(mouseLeaveEventArgs);
		}
		break;
		case WM_SIZE:
		{
			WindowState windowState = DecodeWindowState(_wParam);

			int width = ((int)(short)LOWORD(_lParam));
			int height = ((int)(short)HIWORD(_lParam));

			ResizeEventArgs resizeEventArgs(width, height, windowState);
			pWindow->OnResize(resizeEventArgs);
		}
		break;
		case WM_CLOSE:
		{
			WindowCloseEventArgs windowCloseEventArgs;
			pWindow->OnClose(windowCloseEventArgs);

			// Check to see if the user canceled the close event.
			if (windowCloseEventArgs.ConfirmClose)
			{
				// DestroyWindow( hwnd );
				// Just hide the window.
				// Windows will be destroyed when the application quits.
				pWindow->Hide();
			}
		}
		break;
		case WM_DESTROY:
		{
			std::lock_guard<std::mutex> lock(gs_WindowHandlesMutex);
			WindowMap::iterator         iter = gs_WindowMap.find(_hwnd);
			if (iter != gs_WindowMap.end())
			{
				gs_WindowMap.erase(iter);
			}
		}
		break;
		default:
			return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
		}
	}
	else
	{
		switch (_message)
		{
		case WM_CREATE:
			break;
		default:
			return ::DefWindowProcW(_hwnd, _message, _wParam, _lParam);
		}
	}

	return 0;
}

Application& Application::Create(HINSTANCE _hInst)
{
	if (!gs_Application)
	{
		gs_Application = new Application(_hInst);
	}

	return *gs_Application;
}

void Application::Destroy()
{
	if (gs_Application)
	{
		delete gs_Application;
		gs_Application = nullptr;
	}
	std::cout << "框架类Destroy" << std::endl;
}

Application& Application::Get()
{
	assert(gs_Application);
	return *gs_Application;
}

std::shared_ptr<Window> Application::CreateWindow(const std::wstring& _windowName, int _width, int _height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height) };

	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	uint32_t width = windowRect.right - windowRect.left;
	uint32_t height = windowRect.bottom - windowRect.top;

	int windowX = std::max<int>(0, (screenWidth - (int)width) / 2);
	int windowY = std::max<int>(0, (screenHeight - (int)height) / 2);

	HWND hWnd = CreateWindowW(WINDOW_CLASS_NAME, _windowName.c_str(),
		WS_OVERLAPPEDWINDOW, windowX, windowY,
		width, height, nullptr, nullptr, m_HIntance, nullptr);

	if (!hWnd)
	{
		MessageBoxA(NULL, "无法创建渲染窗口", "错误", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	//创建窗口
	auto pWindow = std::make_shared<MakeWindow>(hWnd, _windowName, _width, _height);
	//将窗口实例加入容器当中
	gs_WindowMap.insert(WindowMap::value_type(hWnd, pWindow));
	gs_WindowByName.insert(windowNameMap::value_type(_windowName, pWindow));

	return pWindow;
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& _windowName) const
{
	std::shared_ptr<Window> window;
	windowNameMap::iterator iter = gs_WindowByName.find(_windowName);
	if (iter != gs_WindowByName.end())
	{
		window = iter->second.lock();
	}
	else
	{
		window = nullptr;
	}

	return window;
}

std::shared_ptr<gainput::InputMap> Application::CreateInputMap(const char* _name /*= nullptr*/)
{
	return std::make_shared<gainput::InputMap>(m_InputManager, _name);
}

int32_t Application::Run()
{
	assert(!m_bIsRuning);

	m_bIsRuning = true;


	MSG msg = {};
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		m_InputManager.HandleMessage(msg);

		// Check to see of the application wants to quit.
		if (m_RequestQuit)
		{
			::PostQuitMessage(0);
			m_RequestQuit = false;
		}
	}

	m_bIsRuning = false;

	return static_cast<int32_t>(msg.wParam);
}

void Application::SetDisplaySize(int _width, int _height)
{
	m_InputManager.SetDisplaySize(_width, _height);
}

void Application::ProcessInput()
{
	m_InputManager.Update();
}

void Application::Stop()
{
	m_RequestQuit = true;
}

Application::Application(HINSTANCE _hIns)
	:m_HIntance(_hIns),
	m_bIsRuning(false),
	m_RequestQuit(false)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
CreateConsole();
#endif

	//初始化输入设备
	m_KeyboardDevice = m_InputManager.CreateDevice<gainput::InputDeviceKeyboard>();
	m_MouseDevice = m_InputManager.CreateDevice<gainput::InputDeviceMouse>();

	m_InputManager.SetDisplaySize(1, 1);


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
}

Application::~Application()
{
	std::cout << "框架类析构" << std::endl;
	gs_WindowByName.clear();
	gs_WindowMap.clear();
}

LRESULT Application::OnWndProc(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam)
{
	auto res = WndProcHandler(_hWnd, _msg, _wParam, _lParam);

	return res ? *res : 0;
}

void Application::OnExit(EventArgs& _e)
{
	Exit(_e);
}

