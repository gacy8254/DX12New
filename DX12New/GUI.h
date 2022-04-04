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
	//������Ϣ����,��ҪApplication����,������IMGUI����������Ϣ
	LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	//��ʼһ���µ�IMGUI���,���޸�IMGUI��Ⱦ�����ĵ�imgui����֮ǰִ��
	void NewFrame();

	//��ȾIMGUI��ָ������ȾĿ����
	void Render(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

	//ע��
	void Destroy();

	//���������С,��window DPI�ı�ʱ����
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

