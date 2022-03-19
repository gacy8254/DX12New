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
	//��̨���������
	static const UINT m_BufferCount = 3;

	//��ȡ���ھ��
	HWND GetWindowHandle() { return m_HWND; }

	//�ݻٴ���
	void Destroy();

	const std::wstring& GetWindowName()const { return m_WindowName; }

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	//vsync���ú���
	bool IsVSync() const { return m_VSync; }
	void SetVSync(bool _vsync) { m_VSync = _vsync; }
	void ToggleVSync() { SetVSync(!m_VSync); }

	//ȫ��
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//��ʾ����
	void Show() { ::ShowWindow(m_HWND, SW_SHOW); }
	void Hide() { ::ShowWindow(m_HWND, SW_HIDE); }

	//���ص�ǰ��̨�������
	UINT GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	//����ͼ�񣬷��ص�ǰ�Ļ��������
	UINT Present();

	//��ȡ��ǰ��RTV������
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

	//��ȡ��̨����������Դ
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	friend class Game;

	friend class Application;

	Window() = delete;
	Window(HWND _hwnd, const std::wstring& _windowName, int _width, int _height, bool _vsync);
	virtual ~Window();

	//����������
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

	//ע��ص�����
	void RegisterCallBacks(std::shared_ptr<Game> _pGame);

	//���ºͻ��ƺ��������ᱻApplication����
	virtual void OnUpdate(UpdateEventArgs& _e);
	virtual void OnRender(RenderEventArgs& _e);

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
	
	//����RTV
	void UpdateRTVs();

private:
	//���ÿ�������
	Window(const Window& copy) = delete;
	Window& operator=(const Window& other) = delete;

	//��������
	std::wstring m_WindowName;
	//���ھ��
	HWND m_HWND;
	//���ھ���,�����л�ȫ��״̬ʱ��¼���ڳߴ�
	RECT m_WindowRect;
	//���ڴ�С
	int m_Width = 1280;
	int m_Height = 720;

	//���ڿ��ƽ������ı���
	bool m_VSync = true;
	bool m_TearingSupported = false;
	//�Ƿ�ȫ��
	bool m_FullScreen = false;

	//������  RTV�Ѻͺ�̨������
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[m_BufferCount];

	UINT m_RTVDescriptorSize;//�������Ĵ�С������ƫ�Ƶ�ַ
	UINT m_CurrentBackBufferIndex;

	//��ʱ��
	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;
	uint64_t m_FrameCounter;

	std::weak_ptr<Game> m_pGame;
};

