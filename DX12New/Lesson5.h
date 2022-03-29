#pragma once
#include "BaseCamera.h"
#include "Light.h"
#include "Application.h"
#include "RenderTarget.h"
#include "CommandList.h"
#include "Device.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include "SwapChain.h"
#include "Window.h"
#include "CommandQueue.h"
#include "RenderTarget.h"
#include "Scene.h"
#include "EffectPSO.h"
#include "SceneVisitor.h"

#include <d3d12.h>
#include <future>
#include <memory>
#include <string>

#include <assimp/Logger.hpp>

class Lesson5
{
public:
	Lesson5(const std::wstring& _name, int _width, int _height, bool _vSync = false);
	~Lesson5();

	//开始主循环
	uint32_t Run();

	//加载所需的内容
	void LoadContent();

	//卸载加载的内容
	void UnLoadContent();

protected:
    //更新游戏逻辑
    void OnUpdate(UpdateEventArgs& e);

    //更改窗口大小
    void OnResize(ResizeEventArgs& e);

    //渲染
    void OnRender();

    //按键按下
    void OnKeyPressed(KeyEventArgs& e);

    //按键松开
    void OnKeyReleased(KeyEventArgs& e);

     //鼠标移动
    virtual void OnMouseMoved(MouseMotionEventArgs& e);

    //DPI改变
    void OnDPIScaleChanged(DPIScaleEventArgs& e);
    
    //渲染UI
    void OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

private:
    //加载场景
    bool LoadScene(const std::wstring& sceneFile);

    //打开文件对话框,并加载新的场景文件
    void OpenFile();

    //加载进度
    bool LoadingProgress(float _loadingProgress);

    void BuildLighting(int numPointLights, int numSpotLights, int numDirectionalLights);

	std::shared_ptr<EffectPSO> m_LightingPSO;
	std::shared_ptr<EffectPSO> m_DecalPSO;
	std::shared_ptr<EffectPSO> m_UnlitPSO;

    std::shared_ptr<Device> m_Device;
    std::shared_ptr<SwapChain> m_SwapChain;
    std::shared_ptr<GUI> m_GUI;
    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Scene> m_Axis;
    std::shared_ptr<Scene> m_Sphere;
    std::shared_ptr<Scene> m_Cone;
    std::shared_ptr<Window> m_Window;

    RenderTarget m_RenderTarget;

    D3D12_RECT m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    BaseCamera m_Camera;
    //Assimp::Logger m_Logger;

    int m_Width;
    int m_Height;
    bool m_VSync;

	// Define some lights.
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight>  m_SpotLights;
	std::vector<DirectionalLight> m_DirectionalLights;

	// Rotate the lights in a circle.
	bool m_AnimateLights;

	bool              m_Fullscreen;
	bool              m_AllowFullscreenToggle;
	bool              m_ShowFileOpenDialog;
	bool              m_CancelLoading;
	bool              m_ShowControls;
	std::atomic_bool  m_IsLoading;
	std::future<bool> m_LoadingTask;
	float             m_LoadingProgress;
	std::string       m_LoadingText;

	float m_FPS;

	float m_Forward;
	float m_Backward;
	float m_Left;
	float m_Right;
	float m_Up;
	float m_Down;

	float m_Pitch;
	float m_Yaw;

	bool m_Shift;
};

