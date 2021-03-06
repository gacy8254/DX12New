#pragma once
#include "BaseCamera.h"
#include "CameraController.h"
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
#include "DeferredGBufferPSO.h"
#include "SceneVisitor.h"
#include "DeferredLightingPSO.h"
#include "SkyCubePSO.h"
#include "NormalVisualizePSO.h"
#include "FinalLDRPSO.h"
#include "IntegrateBRDFPSO.h"
#include "WireframePSO.h"
#include "TAAPSO.h"
#include "ClusterDreferredPSO.h"
#include "ShadowMapPSO.h"
#include "GUI.h"

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
    void TAA(std::shared_ptr<CommandList> _commandList);

    void ClusterLight(std::shared_ptr<CommandList> _commandList);

    void ShadowMap(std::shared_ptr<CommandList> _commandList);

    void PointLightShadowMap(std::shared_ptr<CommandList> _commandList, int& _index);
    void DirectLightShadowMap(std::shared_ptr<CommandList> _commandList, int& _index);

    //加载场景
    bool LoadScene(const std::wstring& sceneFile);

    //打开文件对话框,并加载新的场景文件
    void OpenFile();

    //加载进度
    bool LoadingProgress(float _loadingProgress);

    void BuildLighting(int numPointLights, int numSpotLights, int numDirectionalLights);

    void DrawLightMesh(SceneVisitor& _pass);

    void CreateGBufferRT();

    //准备渲染立方体贴图所需的相机
    void BuildCubemapCamera(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f);

    void BuildShadowMapCamera(const Vector4& _pos, const Vector4& _dir, bool _isPerspective = true);

    //预计算立方体贴图卷积
    void PreIrradiance(std::shared_ptr<CommandList> _commandList);

    void Prefilter(std::shared_ptr<CommandList> _commandList);

    void IntegrateBRDF(std::shared_ptr<CommandList> _commandList);

    void GUILayout(bool* _open);

    void GUITree(bool* _open);

    void BuildScene();

    void BindTransform(float* _pos, float* _rotation, float* _scale, std::shared_ptr<SceneNode> _node);

    void BindMaterial(float* _color, float* _orm, float* _emissive, std::shared_ptr<Actor> _mesh);

    std::shared_ptr<MainPass> mainPassCB;

    //贴图
    std::shared_ptr<Texture> m_CubeMap;
    std::shared_ptr<Texture> m_CubeMap1;
    std::shared_ptr<Texture> m_PrefilterCubeMap;

    std::vector<std::shared_ptr<RenderTarget>> m_ShadowMapRenderTarget;
    std::vector<std::shared_ptr<Texture>> m_ShadowMapTexture;
    std::vector<std::shared_ptr<Texture>> m_DirectLightShadowMapTexture;



    //PSO
    std::unique_ptr<EffectPSO> m_UnlitPso;

    std::unique_ptr<NormalVisualizePSO> m_NormalVisualizePso;
   
    std::unique_ptr<DeferredGBufferPSO> m_GBufferPso;
    std::unique_ptr<DeferredGBufferPSO> m_GBufferDecalPso;

    std::unique_ptr<DeferredLightingPSO> m_DeferredLightingPso;

    std::unique_ptr<SkyCubePSO> m_SkyBoxPso;
    std::unique_ptr<SkyCubePSO> m_PreCalPso;
    std::unique_ptr<SkyCubePSO> m_PrefilterPso;

    std::unique_ptr<FinalLDRPSO> m_LDRPSO;

    std::unique_ptr<IntegrateBRDFPSO> m_IntegrateBRDFPSO;

    std::unique_ptr<ClusterDreferredPSO> m_ClusterDreferredPSO;

    std::unique_ptr<ShadowMapPSO> m_ShadowMapPSO;


    std::unique_ptr<WireframePSO> m_WireframePSO;
    std::unique_ptr<TAAPSO> m_TAAPSO;
    std::shared_ptr<Texture> m_HistoryTexture;
    std::shared_ptr<Texture> m_DepthTexture;

    //设备
    std::shared_ptr<Device> m_Device;
    std::shared_ptr<SwapChain> m_SwapChain;
    std::shared_ptr<GUI> m_GUI;

    //场景模型
    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Scene> m_Axis;
    std::shared_ptr<Scene> m_Sphere;
    std::shared_ptr<Scene> m_Cube;
    std::shared_ptr<Scene> m_Cone;
    std::shared_ptr<Window> m_Window;
    std::shared_ptr<Scene> m_Screen;


    //HDR渲染目标
    RenderTarget m_HDRRenderTarget;

    //盛杜图
    RenderTarget m_DepthRenderTarget;

    //TAA渲染目标,0是当前帧,1是历史帧
    RenderTarget m_TAARenderTarget;

    //LDR渲染目标
    RenderTarget m_LDRRenderTarget;

    //渲染GBuffer的RT
    RenderTarget m_GBufferRenderTarget;
    //预计算立方体贴图卷积的RT
    RenderTarget m_IrradianceRenderTarget;
    RenderTarget m_PrefilterRenderTarget;

    //2D LUT
    RenderTarget m_IntegrateBRDFRenderTarget;

    D3D12_RECT m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    BaseCamera m_Camera;
    CameraController m_CameraController;
    BaseCamera m_CubeMapCamera[6];
    BaseCamera m_ShadowMapCamera;
    //Assimp::Logger m_Logger;

    int m_Width;
    int m_Height;
    bool m_VSync;
    bool m_IsWireFrameMode = false;

	// Define some lights.
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight>  m_SpotLights;
    std::vector<DirectionalLight> m_DirectionalLights;

	bool              m_Fullscreen;
	bool              m_AllowFullscreenToggle;
	bool              m_ShowFileOpenDialog;
	bool              m_CancelLoading;
	bool              m_ShowControls;
    bool              m_TAA = true;
	std::atomic_bool  m_IsLoading;
	std::future<bool> m_LoadingTask;
	float             m_LoadingProgress;
	std::string       m_LoadingText;
    bool              m_ShowTimeWindow;

	float m_FPS;


	// 8x TAA
	inline static const double Halton_2[8] =
	{
		0.0,
		-1.0 / 2.0,
		1.0 / 2.0,
		-3.0 / 4.0,
		1.0 / 4.0,
		-1.0 / 4.0,
		3.0 / 4.0,
		-7.0 / 8.0
	};

	// 8x TAA
	inline static const double Halton_3[8] =
	{
		-1.0 / 3.0,
		1.0 / 3.0,
		-7.0 / 9.0,
		-1.0 / 9.0,
		5.0 / 9.0,
		-5.0 / 9.0,
		1.0 / 9.0,
		7.0 / 9.0
	};



    double GbufferPassTime;
    double PBRPassTime;
    double SkyboxPassTime;
    double TAAPassTime;
    double SDRPassTime;
    //double GbufferPassTime;
    //double GbufferPassTime;
    //double GbufferPassTime;
};

