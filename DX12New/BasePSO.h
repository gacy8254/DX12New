#pragma once
#include "CommandList.h"
#include "Device.h"
#include "helpers.h"
#include "Material.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include "ShaderDefinition.h"
#include <array>

struct ObjectCB
{
	Matrix4 World;
	Matrix4 PreviousWorld;
	Matrix4 InverseTransposeWorld;
	Matrix4 TexcoordTransform;
};

struct MainPass
{
	Matrix4 View;
	Matrix4 InverseView;
	Matrix4 Proj;
	Matrix4 InverseProj;
	Matrix4 UnjitteredProj;
	Matrix4 UnjitteredInverseProj;
	Matrix4 InverseViewProj;
	Matrix4 ViewProj;
	Matrix4 UnjitteredViewProj;
	Matrix4 ViewProjTex;
	Matrix4 PreviousViewProj;
	Vector4 CameraPos;
	float JitterX;
	float JitterY;
	float RTSizeX;
	float RTSizeY;
	float InverseRTSizeX;
	float InverseRTSizeY;
	float TotalTime;
	float DeltaTime;
	float NearZ;
	float FarZ;
	UINT FrameCount;
	float Pad;
};

class BasePSO
{
public:
	BasePSO(std::shared_ptr<Device> _device);
	virtual ~BasePSO();

	const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
	void SetMaterial(const std::shared_ptr<Material>& _material)
	{
		m_Material = _material;
		m_DirtyFlags |= DF_Material;
	}

	void SetObjectCB(std::shared_ptr<ObjectCB> _value)
	{ 
		m_pAlignedObjectCB = _value; 
		m_DirtyFlags |= DF_ObjectCB;
	}

	void SetMainPassCB(std::shared_ptr<MainPass> _value)
	{ 
		m_pAlignedMainPassCB = _value;
		m_DirtyFlags |= DF_MainPassCB;
	}

	//应用到渲染管线上
	virtual void Apply(CommandList& _commandList) = 0;

	static std::array<CD3DX12_STATIC_SAMPLER_DESC, 8> GetStaticSamplers();

protected:
	enum DirtyFlags
	{
		DF_None = 0,
		DF_PointLights = (1 << 0),
		DF_SpotLights = (1 << 1),
		DF_DirectionalLights = (1 << 2),
		DF_Material = (1 << 3),
		DF_ObjectCB = (1 << 4),
		DF_MainPassCB = (1 << 5),
		DF_ShadowMap = (1 << 6),
		DF_DirectLightShadowMap = (1 << 7),
		DF_All = DF_PointLights | DF_SpotLights | DF_DirectionalLights | DF_Material | DF_ObjectCB | DF_MainPassCB | DF_ShadowMap | DF_DirectLightShadowMap
	};

	void BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture, UINT _slot, bool _cubeMap = false);

	std::shared_ptr<Device> m_Device;
	std::shared_ptr<RootSignature> m_RootSignature;
	std::shared_ptr<PipelineStateObject> m_PSO;

	std::shared_ptr<Material> m_Material;

	//默认的空白SRV用于占位贴图槽
	std::shared_ptr<ShaderResourceView> m_DefaultSRV;
	std::shared_ptr<ShaderResourceView> m_DefaultSRVCube;

	std::shared_ptr<ObjectCB> m_pAlignedObjectCB;
	std::shared_ptr<MainPass> m_pAlignedMainPassCB;

	CommandList* m_pPreviousCommandList;

	//有哪些需要绑定的属性
	uint32_t m_DirtyFlags;

};

