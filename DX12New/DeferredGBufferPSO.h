#pragma once
#include "BasePSO.h"

#include "Transform.h"

#include <memory>
#include <vector>

class DeferredGBufferPSO : public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		MatricesCB,  // ConstantBuffer<Matrices> MatCB : register(b0);

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

	void XM_CALLCONV SetWorldMatrix(Matrix4 worldMatrix);
	Matrix4 GetWorldMatrix() const;

	void XM_CALLCONV SetViewMatrix(Matrix4 viewMatrix);
	Matrix4 GetViewMatrix() const;

	void XM_CALLCONV SetProjectionMatrix(Matrix4 projectionMatrix);
	Matrix4 GetProjectionMatrix() const;

	//应用到渲染管线上
	void Apply(CommandList& _commandList);

private:
	bool m_EnableDecal;
};
