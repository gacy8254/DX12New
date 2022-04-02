#pragma once
#include "BasePSO.h"

#include "Transform.h"
#include "ShaderResourceView.h"
#include "Texture.h"

#include <memory>

class SkyCubePSO :
    public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		MatricesCB,  // ConstantBuffer<Matrices> MatCB : register(b0);
		// Pixel shader parameters
		Textures,  // Texture2D HDR       : register( t0 );
		Roughness,	//cbuffer cBuffer : register(b1)
		NumRootParameters
	};

	SkyCubePSO(std::shared_ptr<Device> _device, bool _isPreCal = false, bool _prefilter = false);
	virtual ~SkyCubePSO();

	const std::shared_ptr<Material>& GetMaterial() const;
	void SetMaterial(const std::shared_ptr<Material>& _material);

	void XM_CALLCONV SetWorldMatrix(Matrix4 worldMatrix);
	Matrix4 GetWorldMatrix() const;

	void XM_CALLCONV SetViewMatrix(Matrix4 viewMatrix);
	Matrix4 GetViewMatrix() const;

	void XM_CALLCONV SetProjectionMatrix(Matrix4 projectionMatrix);
	Matrix4 GetProjectionMatrix() const;

	void SetRoughness(float _roughness) 
	{ 
		m_Roughness = _roughness; 
		m_DirtyFlags |= DF_PointLights;
	}

	//应用到渲染管线上
	void Apply(CommandList& _commandList);

private:
	std::shared_ptr<ShaderResourceView> m_SRV;

	float m_Roughness;
	bool m_Prefilter;
};

