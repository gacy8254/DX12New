#pragma once
#include "BasePSO.h"
#include "CommandList.h"
#include "UnorderedAccessView.h"
#include "Light.h"
#include "DescriptorAllocation.h"

class ClusterDreferredPSO : public BasePSO
{
public:
	enum RootParameter
	{
		LightProperites,	//ConstantBuffer<LightProperties> LightPropertiesCB : register( b0 );
		MainPassCB,	//ConstantBuffer<LightProperties> LightPropertiesCB : register( b0 );
		PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
		SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
		LightList,			//RWStructuredBuffer<LightList> gLightList : register(u0);
		NumRootParameters
	};

	ClusterDreferredPSO(std::shared_ptr<Device> _device, UINT _elementNum, UINT _elementSize);
	virtual ~ClusterDreferredPSO() = default;

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

	std::shared_ptr<UnorderedAccessView> GetUAV() { return m_UAV; }
	//应用到渲染管线上
	void Apply(CommandList& _commandList) override;

	void Resize(UINT _elementNum, UINT _elementSize);

private:
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;

	//DescriptorAllocation m_UAV;
	std::shared_ptr<UnorderedAccessView> m_UAV;
};

