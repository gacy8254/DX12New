#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>

#include <Events.h>
#include <HighResolutionClock.h>

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
	void Show() { ::ShowCaret(m_HWND, SW_SHOW); }
	void Hide() { ::ShowCaret(m_HWND, SW_HIDE); }

	//���ص�ǰ��̨�������
	UINT GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	//����ͼ�񣬷��ص�ǰ�Ļ��������
	UINT Present();

	//��ȡ��ǰ��RTV������
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

	//��ȡ��̨����������Դ
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const {return m_BackBuffers[m_CurrentBackBufferIndex]};

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	friend class Game;

	friend class Application;

	Window() = delete;
	Window(HWND _hwnd, std::wstring& _windowName, int _width, int _height, bool _vsync);
	virtual ~Window();

	//����������
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();
	
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
	ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	ComPtr<ID3D12Resource> m_BackBuffers[m_BufferCount];

	UINT m_RTVDescriptorSize;//�������Ĵ�С������ƫ�Ƶ�ַ
	UINT m_CurrentBackBufferIndex;

	//��ʱ��
	high_resolution_clock m_UpdateClock;
	high_resolution_clock m_RenderClock;
	uint64_t m_FrameCounter;

	std::weak_ptr<Game> m_pGame;
};

