#pragma once
#include "Game.h"
#include "Camera.h"
#include "IndexBuffer.h"
#include "Light.h"
#include "Window.h"
#include "Mesh.h"
#include "RenderTarget.h"
#include "RootSignature.h"
#include "Texture.h"
#include "VertexBuffer.h"

#include <DirectXMath.h>

class Lesson3
	:public Game
{
public:
	using super = Game;

	Lesson3(const std::wstring& _name, int _width, int _height, bool _vSync);
	virtual ~Lesson3();


	//加载演示所需的内容
	virtual bool LoadContent() override;

	//卸载加载的内容
	virtual void UnLoadContent() override;

	//更新游戏逻辑
	virtual void OnUpdate(UpdateEventArgs& _e)override;

	//进行渲染
	virtual void OnRender(RenderEventArgs& _e)override;

	//重写输入函数
	virtual void OnKeyPressed(KeyEventArgs& _e)override;
	virtual void OnKeyReleased(KeyEventArgs& _e)override;
	virtual void OnMouseMoved(MouseMotionEventArgs& _e)override;
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e)override;
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e)override;
	virtual void OnMouseWheel(MouseWheelEventArgs& _e)override;
	virtual void OnResize(ResizeEventArgs& _e)override;

	virtual void OnWindowDestroy()override;

private:
	std::unique_ptr<Mesh> m_Cube;
	std::unique_ptr<Mesh> m_Sphere;
	std::unique_ptr<Mesh> m_Cone;
	std::unique_ptr<Mesh> m_Torus;
	std::unique_ptr<Mesh> m_Plane;
	std::unique_ptr<Mesh> m_SkyBox;

	Texture m_DefaultTexture;
	Texture m_DirectXTexture;
	Texture m_EarthTexture;
	Texture m_MonalisaTexture;
	Texture m_GraceCathedralTexture;
	Texture m_GraceCathedralCubemap;

	RenderTarget m_SDRRenderTarget;
	RenderTarget m_HDRRenderTarget;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SkyPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_HDRPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SDRPSO;

	dx12lib::RootSignature m_SkyboxRootSignature;
	dx12lib::RootSignature m_HDRRootSignature;
	dx12lib::RootSignature m_SDRRootSignature;

	//视口设置
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Rect;

	Camera m_Camera;
	struct alignas(16) CameraData
	{
		DirectX::XMVECTOR m_InitialCamPos;
		DirectX::XMVECTOR m_InitialCamRot;
		float m_InitialFov;
	};
	CameraData* m_pAlignedCameraDate;

	float m_Forward;
	float m_Backward;
	float m_Left;
	float m_Right;
	float m_Up;
	float m_Down;

	float m_Pitch;
	float m_Yaw;

	bool m_AnimaeLights;
	bool m_Shift;

	int m_Width;
	int m_Height;

	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;
};

