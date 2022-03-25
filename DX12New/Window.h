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
	//��̨���������
	static const UINT m_BufferCount = 3;

	//��ȡ���ھ��
	HWND GetWindowHandle() { return m_HWND; }

	float GetDPIScaling() const { return m_DPIscaling; }

	const std::wstring& GetWindowName()const { return m_WindowName; }

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	//���ô��ڱ���
	void SetWindowTitle(const std::wstring& _title);
	const std::wstring& GetWindowTitle() const { return m_Title; }

	//ȫ��
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//��ʾ����
	void Show() { ::ShowWindow(m_HWND, SW_SHOW); }
	void Hide() { ::ShowWindow(m_HWND, SW_HIDE); }

	//������Ҫ����ʱ������
	UpdateEvent Update;

	//DPI�ı�ʱ����
	DPIScaleEvent DPIScaleChanged;

	//���ڹر�ʱ����
	WindowCloseEvent Close;

	//�ı䴰�ڴ�Сʱ����
	ResizeEvent Resize;

	//��С��ʱ����
	ResizeEvent Minimized;

	//���ʱ����
	ResizeEvent Maximized;

	//���ڻָ�ʱ����
	ResizeEvent Restored;

	//���̰�������ʱ����
	KeyboardEvent KeyPressed;

	//���̰����ɿ�ʱ����
	KeyboardEvent KeyReleased;

	//��ü��̽���ʱ����
	Event KeyboardFocus;

	//��ʧ����ʱ����
	Event KeyboardBlur;

	//����ƶ�ʱ����
	MouseMotionEvent MouseMoved;

	//�����봰��ʱ����
	MouseMotionEvent MouseEnter;

	//��갴��ʱ����
	MouseButtonEvent MouseButtonPressed;

	//��갴������ʱ����
	MouseButtonEvent MouseButtonReleased;

	//����ת��ʱ����
	MouseWheelEvent MouseWheel;

	//����뿪����ʱ����
	Event MouseLeave;

	//�����꽹��ʱ����
	Event MouseFocus;

	//��ʧ���ʱ����
	Event MouseBlur;

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	friend class Game;

	friend class Application;

	Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync);
	virtual ~Window();

	//���ºͻ��ƺ��������ᱻApplication����
	virtual void OnUpdate(UpdateEventArgs& _e);

	//���ڹر�
	virtual void OnClose(WindowCloseEventArgs& _e);

	//�ָ�����
	virtual void OnRestored(ResizeEventArgs& _e);

	//��С��
	virtual void OnMinimized(ResizeEventArgs& _e);

	//���
	virtual void OnMaximized(ResizeEventArgs& _e);

	//DPI�仯
	virtual void OnDPIScaaleChanged(DPIScaleEventArgs& _e);

	//���̾۽��ڴ���
	virtual void OnKetboardFocus(EventArgs& _e);
	//��ʧ����
	virtual void OnKetboardBlur(EventArgs& _e);

	//�����봰��
	virtual void OnMouseEnter(MouseMotionEventArgs& _e);
	//����뿪
	virtual void OnMouseLeave(EventArgs& _e);

	//���ڽ��ܵ������Ϣ
	virtual void OnMouseFocus(EventArgs& _e);
	//���ڶ�ʧ���
	virtual void OnMouseBlur(EventArgs& _e);

	//���̰���
	virtual void OnKeyPressed(KeyEventArgs& _e);
	virtual void OnKeyReleased(KeyEventArgs& _e);

	//����¼�
	virtual void OnMouseMoved(MouseMotionEventArgs& _e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e);
	virtual void OnMouseWheel(MouseWheelEventArgs& _e);

	//���ڴ�С����
	virtual void OnResize(ResizeEventArgs& _e);
	
private:
	//��������
	std::wstring m_WindowName;
	std::wstring m_Title;
	//���ھ��
	HWND m_HWND;
	//���ھ���,�����л�ȫ��״̬ʱ��¼���ڳߴ�
	RECT m_WindowRect;
	//����Ƿ�λ�ڴ�����
	bool m_bInClientRect;

	//���ڽ��յ����̽���
	bool m_bHasKeyboradFocus;

	//���ڴ�С
	int m_Width = 1280;
	int m_Height = 720;

	//���ڵ�DPI����
	float m_DPIscaling;

	//�Ƿ���С�������
	bool m_IsMinimized;
	bool m_IsMaximized;

	//�Ƿ�ȫ��
	bool m_FullScreen;

	HighResolutionTimer m_Timer;

	int m_PreviousMouseX;
	int m_PreviousMouseY;
};

