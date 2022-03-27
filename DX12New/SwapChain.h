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


	//全屏
	bool IsFullScreen() const { return m_FullScreen; }
	void SetFullScreen(bool _fullScreen);
	void ToggleFullScreen() { SetFullScreen(!m_FullScreen); }

	//VSync
	bool GetVSync() const { return m_VSync; }
	void SetVSync(bool _vSync) { m_VSync = _vSync; }
	void ToggleVSync() { SetVSync(!m_VSync); }

	//检查是否支持防撕裂
	bool IsTearingSupported() const { return m_TearingSupported; }

	//阻拦当前线程，等待交换链完成图像的呈现
	//在更新循环开始时这么做可以改善输入延迟
	void WaitForSwapChain();

	//更新交换链后台缓冲的尺寸
	void Resize(uint32_t _width, uint32_t _height);

	//获取当前窗口的渲染目标
	//这个方法应当每一帧调用一次，因为颜色附着点根据当前窗口的后台缓冲区变换
	const RenderTarget& GetRenderTarget() const;

	//将当前的后台缓冲呈现到屏幕上
	//@Param 要复制到后台缓冲区的纹理
	//默认情况下是空，在这种情况下不会执行复制
	//使用GetRenderTarget获取当前窗口的渲染目标
	UINT Present(const std::shared_ptr<Texture>& _texture = nullptr);

	//获取后台缓冲区的格式
	DXGI_FORMAT GetRenderTargetFormat() const { return m_RenderTargetFormat; }

	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapChain() const { return m_SwapChain; }

protected:
	SwapChain(Device& _device, HWND _hwnd, DXGI_FORMAT _rendertargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM);
	virtual ~SwapChain();

	//更新RTV
	void UpdateRenderTargetViews();

private:
	//窗口句柄
	HWND m_HWnd;

	//渲染目标格式
	DXGI_FORMAT m_RenderTargetFormat;

	//设备
	Device& m_Device;

	//命令队列
	CommandQueue& m_CommandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
	std::shared_ptr<Texture> m_BackBufferTextures[BufferCount];
	mutable RenderTarget m_RenderTarget;

	//当前后台缓冲的索引
	UINT   m_CurrentBackBufferIndex;
	//围栏值
	UINT64 m_FenceValues[BufferCount];

	//等待对象的句柄，用于在呈现图像前等待交换链
	HANDLE m_hFrameLatencyWaitableObject;

	//宽高
	uint32_t m_Width;
	uint32_t m_Height;

	//功能开启与否
	bool m_VSync;
	bool m_TearingSupported;
	bool m_FullScreen;
};

