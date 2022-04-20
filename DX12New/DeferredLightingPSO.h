#pragma once
#include "BasePSO.h"

#include "Light.h"
#include "VertexType.h"
#include "Transform.h"
#include "Texture.h"
#include "CommandList.h"

#include <memory>
#include <vector>
#include "ShaderResourceView.h"

struct LightList
{
	unsigned int PointLightIndices[MAX_GRID_POINT_LIGHT_NUM];
	unsigned int NumPointLights;
	unsigned int SpotLightIndices[MAX_GRID_SPOTLIGHT_NUM];
	unsigned int NumSpotlights;
};

class DeferredLightingPSO : public BasePSO
{
public:


	enum RootParameters
	{
		LightPropertiesCB,		// ConstantBuffer<LightProperties> LightPropertiesCB					: register( b0 );
		MainPassCB,				// ConstantBuffer<LightProperties> LightPropertiesCB					: register( b0 );
		CameraPosCB,			// ConstantBuffer<float4> CameraPos										: register( b2 );

		PointLights,			// StructuredBuffer<PointLight> PointLights								: register( t0 );
		SpotLights,				// StructuredBuffer<SpotLight> SpotLights								: register( t1 );
		DirectionalLights,		// StructuredBuffer<DirectionalLight> DirectionalLights					: register( t2 )
		LightsList,				// StructuredBuffer<LightList> LightsList								: register(t3);

		Textures,				// Texture2D AlbedoText,												: register (t4)
								// Texture2D NormalText,												: register (t5)
								// Texture2D ORMText,													: register (t6)
								// Texture2D EmissiveText,												: register (t7)
								// Texture2D WorldPosText,												: register (t8)
								// Texture2D IrradianceText,											: register (t9)
								// Texture2D PrefilterText												: register (t10)
								// Texture2D IntegrateBRDFText,											: register (t11)
								// Texture2D DepthText,													: register (t12)
		ShadowMaps,				// TextureCube<float4> ShadowMapText[MAX_POINT_LIGHT_SHADOWMAP_NUM],	: register (t13)
		DirectLightShadowMap,	// Texture2D DirectLightShadowMapText[MAX_DIRECT_LIGHT_SHADOWMAP_NUM]	: register(t14);
		NumRootParameters
	};

	enum GBufferTexture
	{
		AlbedoText,
		NormalText,
		ORMText,
		EmissiveText,
		WorldPosText,
		IrradianceText,
		PrefilterText,
		IntegrateBRDFText,
		DepthText,
		NumTextures
	};

	DeferredLightingPSO(std::shared_ptr<Device> _device, bool _enableLighting);
	virtual ~DeferredLightingPSO();

	void SetCameraPos(Vector4 _pos) { m_CameraPos = _pos; }

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

	void SetLightList(std::shared_ptr<ShaderResourceView> _lightList, UINT _num, UINT _size) 
	{ 
		m_LightList = _lightList; 
		m_ElementNum = _num;
		m_ElementSize = _size;
	}

	//const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
	void SetTexture(std::vector<std::shared_ptr<Texture>>& _textures)
	{
		m_Textures = _textures;
		m_DirtyFlags |= DF_Material;
	}

	void SetShadowMap(std::vector<std::shared_ptr<Texture>>& _textures)
	{
		m_ShadowMap = _textures;
		m_DirtyFlags |= DF_ShadowMap;
	}

	void SetDirectLightShadowMap(std::vector<std::shared_ptr<Texture>>& _textures)
	{
		m_DirectLightShadowMap = _textures;
		m_DirtyFlags |= DF_DirectLightShadowMap;
	}

	//应用到渲染管线上
	void Apply(CommandList& _commandList) override;

private:

	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;
	std::vector<DirectionalLight> m_DirectionalLights;

	std::vector<std::shared_ptr<Texture>> m_Textures;
	std::vector<std::shared_ptr<Texture>> m_ShadowMap;
	std::vector<std::shared_ptr<Texture>> m_DirectLightShadowMap;

	std::shared_ptr<ShaderResourceView> m_LightList;
	UINT m_ElementNum;
	UINT m_ElementSize;

	Vector4 m_CameraPos;

	bool m_EnableLights;
};

