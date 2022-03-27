#pragma once

#include <dxgi1_5.h>     // For IDXGISwapChain4
#include <wrl/client.h>  // For Microsoft::WRL::ComPtr

#include <memory>  // For std::shared_ptr

#include "RenderTarget.h"

class CommandQueue;
class Device;
class Texture;



class SwapChain
{
public:
	static const UINT BufferCount = 3;


	//ȫ��
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//VSync
	bool GetVSync() const { return m_VSync; }
	void SetVSync(bool _vSync) { m_VSync = _vSync; }
	void ToggleVSync() { SetVSync(!m_VSync); }

	//����Ƿ�֧�ַ�˺��
	bool IsTearingSupported() const { return m_TearingSupported; }

	//������ǰ�̣߳��ȴ����������ͼ��ĳ���
	//�ڸ���ѭ����ʼʱ��ô�����Ը��������ӳ�
	void WaitForSwapChain();

	//���½�������̨����ĳߴ�
	void Resize(uint32_t _width, uint32_t _height);

	//��ȡ��ǰ���ڵ���ȾĿ��
	//�������Ӧ��ÿһ֡����һ�Σ���Ϊ��ɫ���ŵ���ݵ�ǰ���ڵĺ�̨�������任
	const RenderTarget& GetRenderTarget() const;

	//����ǰ�ĺ�̨������ֵ���Ļ��
	//@Param Ҫ���Ƶ���̨������������
	//Ĭ��������ǿգ�����������²���ִ�и���
	//ʹ��GetRenderTarget��ȡ��ǰ���ڵ���ȾĿ��
	UINT Present(const std::shared_ptr<Texture>& _texture = nullptr);

	//��ȡ��̨�������ĸ�ʽ
	DXGI_FORMAT GetRenderTargetFormat() const { return m_RenderTargetFormat; }

	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapChain() const { return m_SwapChain; }

protected:
	SwapChain(Device& _device, HWND _hwnd, DXGI_FORMAT _rendertargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM);
	virtual ~SwapChain();

	//����RTV
	void UpdateRenderTargetViews();

private:
	//���ھ��
	HWND m_HWnd;

	//��ȾĿ���ʽ
	DXGI_FORMAT m_RenderTargetFormat;

	//�豸
	Device& m_Device;

	//�������
	CommandQueue& m_CommandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
	std::shared_ptr<Texture> m_BackBufferTextures[BufferCount];
	mutable RenderTarget m_RenderTarget;

	//��ǰ��̨���������
	UINT   m_CurrentBackBufferIndex;
	//Χ��ֵ
	UINT64 m_FenceValues[BufferCount];

	//�ȴ�����ľ���������ڳ���ͼ��ǰ�ȴ�������
	HANDLE m_hFrameLatencyWaitableObject;

	//���
	uint32_t m_Width;
	uint32_t m_Height;

	//���ܿ������
	bool m_VSync;
	bool m_TearingSupported;
	bool m_FullScreen;
};

