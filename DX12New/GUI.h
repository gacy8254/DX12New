#pragma once
#include "imgui.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  // For HWND

#include <memory>  // For std::shared_ptr

class CommandList;
class Device;
class PipelineStateObject;
class RenderTarget;
class RootSignature;
class ShaderResourceView;
class Texture;

class GUI
{
public:
	//窗口消息处理,需要Application调用,以允许IMGUI处理输入消息
	LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	//开始一个新的IMGUI框架,在修改IMGUI渲染上下文的imgui函数之前执行
	void NewFrame();

	//渲染IMGUI到指定的渲染目标上
	void Render(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

	//注销
	void Destroy();

	//缩放字体大小,在window DPI改变时调用
	void SetScaling(float scale);

protected:
	GUI(Device& _device, HWND _hwnd, const RenderTarget& _rt);
	virtual ~GUI();

private:
	Device& m_Device;
	HWND m_HWND;
	ImGuiContext* m_pImguiContextl;
	std::shared_ptr<Texture> m_FontTexture;
	std::shared_ptr<ShaderResourceView> m_FontSRV;
	std::shared_ptr<RootSignature> m_RootSignature;
	std::shared_ptr<PipelineStateObject> m_PSO;
};

