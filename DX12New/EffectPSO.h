#pragma once
#include "BasePSO.h"

#include "Light.h"
#include "VertexType.h"
#include "Transform.h"

#include <memory>
#include <vector>

class EffectPSO : public BasePSO
{
public:
	struct LightProperties
	{
		uint32_t NumPointLights;
		uint32_t NumSpotLights;
		uint32_t NumDirectionalLights;
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
	void Apply(CommandList& _commandList) override;

private:

	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;
	std::vector<DirectionalLight> m_DirectionalLights;

	bool m_EnableLights;
	bool m_EnableDecal;
};

