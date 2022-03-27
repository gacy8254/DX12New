#include "ByteAddressBuffer.h"
#include "D3D12LibPCH.h"

ByteAddressBuffer::ByteAddressBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource)
	:Buffer(_device, _resource)
{}

ByteAddressBuffer::ByteAddressBuffer(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc)
	:Buffer(_device, _resourceDesc)
{}
