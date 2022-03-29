#pragma once
#include "Light.h"
#include "CommandList.h"
#include "Device.h"
#include "helpers.h"
#include "Material.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include "VertexType.h"

//#include <DirectXMath.h>
#include "Transform.h"

#include <memory>
#include <vector>

class EffectPSO
{
public:
	struct LightProperties
	{
		uint32_t NumPointLights;
		uint32_t NumSpotLights;
		uint32_t NumDirectionalLights;
	};

	struct alignas(16) Matrices
	{
		Matrix4 ModelMatrix;
		Matrix4 ModelViewMatrix;
		Matrix4 InverseTransposeModelViewMatrix;
		Matrix4 ModelViewProjectionMatrix;
	};

	enum RootParameters
	{
		// Vertex shader parameter
		MatricesCB,  // ConstantBuffer<Matrices> MatCB : register(b0);

		// Pixel shader parameters
		MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
		LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );

		PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
		SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
		DirectionalLights,  // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 )

		Textures,  // Texture2D AmbientTexture       : register( t3 );
				   // Texture2D EmissiveTexture : register( t4 );
				   // Texture2D DiffuseTexture : register( t5 );
				   // Texture2D SpecularTexture : register( t6 );
				   // Texture2D SpecularPowerTexture : register( t7 );
				   // Texture2D NormalTexture : register( t8 );
				   // Texture2D BumpTexture : register( t9 );
				   // Texture2D OpacityTexture : register( t10 );
				   NumRootParameters
	};

	EffectPSO(std::shared_ptr<Device> _device, bool _enableLighting, bool _enableDecal);
	virtual ~EffectPSO();

	const std::vector<PointLight>& GetPointLights() const { return m_PointLights; }
	void SetPointLights(const std::vector<PointLight>& _pointLights) 
	{ 
		m_PointLights = _pointLights; 
		m_DirtyFlags |= DF_PointLights; 
	}

	const std::vector<SpotLight>& GetSpotLights() const { return m_SpotLights; }
	void SetSpotLights(const std::vector<SpotLight>& _spotLights) 
	{ 
		m_SpotLights = _spotLights; 
		m_DirtyFlags |= DF_SpotLights; 
	}

	const std::vector<DirectionalLight>& GetDirectionalLights() const { return m_DirectionalLights; }
	void SetDirectionalLights(const std::vector<DirectionalLight>& _directionalLights) 
	{ 
		m_DirectionalLights = _directionalLights; 
		m_DirtyFlags |= DF_DirectionalLights; 
	}

	const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
	void SetMaterial(const std::shared_ptr<Material>& _material) 
	{ 
		m_Material = _material; 
		m_DirtyFlags |= DF_Material;
	}

	void XM_CALLCONV SetWorldMatrix(Matrix4 worldMatrix) 
	{ 
		m_pAlignedMVP->World = worldMatrix; 
		m_DirtyFlags |= DF_Matrices; 
	}
	Matrix4 GetWorldMatrix() const { return m_pAlignedMVP->World; }

	void XM_CALLCONV SetViewMatrix(Matrix4 viewMatrix) 
	{ 
		m_pAlignedMVP->View = viewMatrix; 
		m_DirtyFlags |= DF_Matrices; 
	}
	Matrix4 GetViewMatrix() const { return m_pAlignedMVP->View; }

	void XM_CALLCONV SetProjectionMatrix(Matrix4 projectionMatrix) 
	{ 
		m_pAlignedMVP->Projection = projectionMatrix; 
		m_DirtyFlags |= DF_Matrices; 
	}
	Matrix4 GetProjectionMatrix() const { return m_pAlignedMVP->Projection; }

	//应用到渲染管线上
	void Apply(CommandList& _commandList);

private:
	enum DirtyFlags
	{
		DF_None = 0,
		DF_PointLights = (1 << 0),
		DF_SpotLights = (1 << 1),
		DF_DirectionalLights = (1 << 2),
		DF_Material = (1 << 3),
		DF_Matrices = (1 << 4),
		DF_All = DF_PointLights | DF_SpotLights | DF_DirectionalLights | DF_Material | DF_Matrices
	};

	struct alignas(16) MVP
	{
		Matrix4 World;
		Matrix4 View;
		Matrix4 Projection;
	};

	inline void BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture);

	std::shared_ptr<Device> m_Device;
	std::shared_ptr<RootSignature> m_RootSignature;
	std::shared_ptr<PipelineStateObject> m_PSO;

	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;
	std::vector<DirectionalLight> m_DirectionalLights;

	std::shared_ptr<Material> m_Material;

	//默认的空白SRV用于占位贴图槽
	std::shared_ptr<ShaderResourceView> m_DefaultSRV;

	MVP* m_pAlignedMVP;

	CommandList* m_pPreviousCommandList;

	//有哪些需要绑定的属性
	uint32_t m_DirtyFlags;

	bool m_EnableLights;
	bool m_EnableDecal;
};

