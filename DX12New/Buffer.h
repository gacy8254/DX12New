#pragma once
#include "Resource.h"

#include "Device.h"


class Buffer : public Resource
{
public:
protected:
	Buffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource);
	Buffer(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc);
};

