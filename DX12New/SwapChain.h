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
protected:
	SwapChain(Device& _device, HWND _hwnd, DXGI_FORMAT _rendertargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM);
	virtual ~SwapChain();

private:
	HWND m_hWnd;

	DXGI_FORMAT m_RenderTargetFormat;

	Device& m_Device;
};

