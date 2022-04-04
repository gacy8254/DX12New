#pragma once
#include "BasePSO.h"
#include <memory>
#include <vector>
#include "Transform.h"

class WireframePSO :
    public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		MatricesCB,  // ConstantBuffer<Matrices> MatCB : register(b0);
		NumRootParameters
	};

	WireframePSO(std::shared_ptr<Device> _device);
	virtual ~WireframePSO();

	void XM_CALLCONV SetWorldMatrix(Matrix4 worldMatrix);
	Matrix4 GetWorldMatrix() const;

	void XM_CALLCONV SetViewMatrix(Matrix4 viewMatrix);
	Matrix4 GetViewMatrix() const;

	void XM_CALLCONV SetProjectionMatrix(Matrix4 projectionMatrix);
	Matrix4 GetProjectionMatrix() const;

	//应用到渲染管线上
	void Apply(CommandList& _commandList);
};

