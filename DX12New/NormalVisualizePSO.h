#pragma once
#include "BasePSO.h"
#include "RenderTarget.h"
#include "Transform.h"

#include <memory>
#include <vector>


class NormalVisualizePSO :
    public BasePSO
{
public:
	enum RootParameters
	{
		// Vertex shader parameter
		ObjectCB,  // cbuffer ObjectCB : register(b0);
		MainPassCB,  // cbuffer MainPassCB : register(b1);
		NumRootParameters
	};

	NormalVisualizePSO(std::shared_ptr<Device> _device);
	virtual ~NormalVisualizePSO();

	//应用到渲染管线上
	void Apply(CommandList& _commandList);
};

