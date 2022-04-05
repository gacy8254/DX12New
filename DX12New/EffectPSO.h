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
		ObjectCB,  // cbuffer ObjectCB : register(b0);
		MainPassCB,  // cbuffer MainPassCB : register(b1);

		// Pixel shader parameters
		MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
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

	//应用到渲染管线上
	void Apply(CommandList& _commandList) override;

private:

	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;
	std::vector<DirectionalLight> m_DirectionalLights;

	bool m_EnableLights;
	bool m_EnableDecal;
};

