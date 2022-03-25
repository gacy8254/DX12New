#include "Buffer.h"

Buffer::Buffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource)
	:Resource(_device, _resource)
{}

Buffer::Buffer(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc)
	:Resource(_device, _resourceDesc)
{}
