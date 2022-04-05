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
		ObjectCB,  // cbuffer ObjectCB : register(b0);
		MainPassCB,  // cbuffer MainPassCB : register(b1);
		NumRootParameters
	};

	WireframePSO(std::shared_ptr<Device> _device);
	virtual ~WireframePSO();

	//Ӧ�õ���Ⱦ������
	void Apply(CommandList& _commandList);
};

