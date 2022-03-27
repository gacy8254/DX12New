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

	//��ȡ���̵��豸ID
	gainput::DeviceId GetKeyboardId() const { return m_KeyboardDevice; }

	//��ȡ�����豸ID
	gainput::DeviceId GetMouseId() const { return m_MouseDevice; }

	//��ȡһ�������豸
	template<class T>
	T* GetDevice(gainput::DeviceId _deviceId) const
	{
		static_assert(std::is_base_of_v<gainput::InputDevice, T>);
		return static_cast<T*>(m_InputManager.GetDevice(_deviceId));
	}

	//����һ������ͼ
	std::shared_ptr<gainput::InputMap> CreateInputMap(const char* _name = nullptr);

	//��ʼִ��,���ش������
	int32_t Run();

	//֪ͨ���������,���ڴ�С�ı�
	//����gainput��׼����������������
	//ֻ�ڵ�������ˢ�´�Сʱ����
	void SetDisplaySize(int _width, int _height);

	//���������¼�
	void ProcessInput();

	//ֹͣ����
	void Stop();

	//Ҫ֧���ȼ����޸Ĺ����ļ�,����ע��һ��·��,���ڼ����ļ��޸�֪ͨ
	//�ļ�����֪ͨͨ��Application::FileChange����
	//void RegisterDirectoryChangeListener(const std::wstring& _dir, bool _recursive = true);

	//����һ����Ⱦ����ʵ��
	//_windowName �����ƽ������ڴ��ڵı������У�������Ӧ����Ψһ��
	//���ش����Ĵ���ʵ��
	std::shared_ptr<Window> CreateWindow(const std::wstring& _windowName, int _width, int _height);

	//ͨ�����ֻ�ȡ����ʵ��
	std::shared_ptr<Window> GetWindowByName(const std::wstring& _windowName) const;

	//����Ϣ���͵�����ʱ����
	WndProcEvent WndProcHandler;

	//���ļ��޸�ʱ����
	FileChangeEvent FileChanged;

	//�������ʱ����
	Event Exit;

protected:
	friend LRESULT CALLBACK WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam);

	//����Ӧ�ó���ʵ��
	Application(HINSTANCE _hIns);

	virtual ~Application();

	//��⵽�ļ��޸�
	//virtual void OnFileChange(FileChangedEventArgs& _e);

	//������Ϣ������
	virtual LRESULT OnWndProc(HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam);

	virtual void OnExit(EventArgs& _e);

private:
	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&) = delete;
	Application& operator=(Application&&) = delete;

	//Ŀ¼�����������߳���ڵ㺯��
	//void CheckFileChanges();

	HINSTANCE m_HIntance;

	//�����豸����
	gainput::InputManager m_InputManager;
	gainput::DeviceId     m_KeyboardDevice;
	gainput::DeviceId     m_MouseDevice;

	//����������ʱΪ��
	std::atomic_bool m_bIsRuning;
	//�Ƿ�Ӧ���˳�����
	std::atomic_bool m_RequestQuit;

};

