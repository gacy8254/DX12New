#pragma once
#include "BasePSO.h"

#include "RenderTarget.h"
#include "Transform.h"

#include <memory>
#include <vector>

class DeferredGBufferPSO : public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		ObjectCB,  // cbuffer ObjectCB : register(b0);
		MainPassCB,  // cbuffer MainPassCB : register(b1);

		// Pixel shader parameters
		MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
		Textures,  // Texture2D AmbientTexture       : register( t0 );
				   // Texture2D EmissiveTexture : register( t1 );
				   // Texture2D DiffuseTexture : register( t2 );
				   // Texture2D SpecularTexture : register( t3 );
				   // Texture2D SpecularPowerTexture : register( t4 );
				   // Texture2D NormalTexture : register( t5 );
				   // Texture2D BumpTexture : register( t6 );
				   // Texture2D OpacityTexture : register( t7 );
		NumRootParameters
	};

	DeferredGBufferPSO(std::shared_ptr<Device> _device, bool _enableDecal);
	virtual ~DeferredGBufferPSO();

	const std::shared_ptr<Material>& GetMaterial() const;
	void SetMaterial(const std::shared_ptr<Material>& _material);

	//应用到渲染管线上
	void Apply(CommandList& _commandList);

private:
	bool m_EnableDecal;

};
