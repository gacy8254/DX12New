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

	//��ʼ��ѭ��
	uint32_t Run();

	//�������������
	void LoadContent();

	//ж�ؼ��ص�����
	void UnLoadContent();

protected:
    //������Ϸ�߼�
    void OnUpdate(UpdateEventArgs& e);

    //���Ĵ��ڴ�С
    void OnResize(ResizeEventArgs& e);

    //��Ⱦ
    void OnRender();

    //��������
    void OnKeyPressed(KeyEventArgs& e);

    //�����ɿ�
    void OnKeyReleased(KeyEventArgs& e);

     //����ƶ�
    virtual void OnMouseMoved(MouseMotionEventArgs& e);

    //DPI�ı�
    void OnDPIScaleChanged(DPIScaleEventArgs& e);
    
    //��ȾUI
    void OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

private:
    //���س���
    bool LoadScene(const std::wstring& sceneFile);

    //���ļ��Ի���,�������µĳ����ļ�
    void OpenFile();

    //���ؽ���
    bool LoadingProgress(float _loadingProgress);

    void BuildLighting(int numPointLights, int numSpotLights, int numDirectionalLights);

    void DrawLightMesh(SceneVisitor& _pass);

    void CreateGBufferRT();

    void DrawSphere(SceneVisitor& _pass, bool isNormal);

    //׼����Ⱦ��������ͼ��������
    void BuildCubemapCamera();

    //Ԥ������������ͼ���
    void PreIrradiance(std::shared_ptr<CommandList> _commandList);

    void Prefilter(std::shared_ptr<CommandList> _commandList);

    void IntegrateBRDF(std::shared_ptr<CommandList> _commandList);

    //��ͼ
    std::shared_ptr<Texture> m_CubeMap;
    std::shared_ptr<Texture> m_CubeMap1;
    std::shared_ptr<Texture> m_PrefilterCubeMap;
    std::shared_ptr<Texture> m_LUT;

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

    //�豸
    std::shared_ptr<Device> m_Device;
    std::shared_ptr<SwapChain> m_SwapChain;
    std::shared_ptr<GUI> m_GUI;

    //����ģ��
    std::shared_ptr<Scene> m_Scene;
    std::shared_ptr<Scene> m_Axis;
    std::shared_ptr<Scene> m_Sphere;
    std::shared_ptr<Scene> m_Cube;
    std::shared_ptr<Scene> m_Cone;
    std::shared_ptr<Window> m_Window;

    //HDR��ȾĿ��
    RenderTarget m_HDRRenderTarget;

    //LDR��ȾĿ��
    RenderTarget m_LDRRenderTarget;

    //��ȾGBuffer��RT
    RenderTarget m_GBufferRenderTarget;
    //Ԥ������������ͼ�����RT
    RenderTarget m_IrradianceRenderTarget;
    RenderTarget m_PrefilterRenderTarget;

    //2D LUT
    RenderTarget m_IntegrateBRDFRenderTarget;

    D3D12_RECT m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    BaseCamera m_Camera;
    CameraController m_CameraController;
    BaseCamera m_CubeMapCamera[6];
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
};

