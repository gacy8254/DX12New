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

	//����Ƿ�֧��VSync
	bool IsTearingSupported() const { return m_TearingSupported; }

	//����һ����Ⱦ����ʵ��
	//_windowName �����ƽ������ڴ��ڵı������У�������Ӧ����Ψһ��
	//���ش����Ĵ���ʵ��
	std::shared_ptr<Window> CreateRenderWindow(const std::wstring& _windowName, int _width, int _height, bool _vSync = true);

	//���ٴ���
	void DestroyWindow(const std::wstring& _windowName);
	void DestroyWindow(std::shared_ptr<Window> _window);
	//ͨ�����ֻ�ȡ����ʵ��
	std::shared_ptr<Window> GetWindowByName(const std::wstring& _windowName);

	//���г���ѭ������Ϣ����
	//���ش������
	int Run(std::shared_ptr<Game> pGame);

	//�����˳����ر����д���
	void Quit(int _exitCode = 0);

	//��ȡ�豸
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const { return m_Device; }

	//��ȡ���У���Ч��Type��
	//D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
	//D3D12_COMMAND_LIST_TYPE_COMPUTE : Can be used for dispatch or copy commands.
	//D3D12_COMMAND_LIST_TYPE_COPY : Can be used for copy commands.
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

	//ˢ�¶���
	void Flush();

	//���� ��������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT _numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE _type);
	//��ȡ�������Ĵ�С
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE _type) const;

	static uint64_t GetFrameCount();

protected:
	//����Ӧ�ó���ʵ��
	Application(HINSTANCE _hIns);

	virtual ~Application();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool _useWarp);
	Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> _adapter);

	//����Ƿ�֧��VSync
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

