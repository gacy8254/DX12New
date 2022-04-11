#pragma once
#include "BasePSO.h"

#include "Transform.h"
#include "ShaderResourceView.h"
#include "Texture.h"

#include <memory>

class ShadowMapPSO :public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		ObjectCB,  // cbuffer ObjectCB : register(b0);
		MainPassCB,  // cbuffer MainPassCB : register(b1);
		//lightPosCB,  // cbuffer lightPosCB : register(b2);
		NumRootParameters
	};

	ShadowMapPSO(std::shared_ptr<Device> _device, bool _isDecal = false);
	virtual ~ShadowMapPSO();

	//void SetLightPos(Vector4 _pos)
	//{
	//	m_LightPos = _pos;
	//	m_DirtyFlags |= DF_PointLights;
	//}

	//应用到渲染管线上
	void Apply(CommandList& _commandList);

private:
	std::shared_ptr<ShaderResourceView> m_SRV;
	
	bool m_IsDecal;
};

