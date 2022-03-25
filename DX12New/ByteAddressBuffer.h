#pragma once
#include "Buffer.h"
#include "DescriptorAllocation.h"

#include "d3dx12.h"

class ByteAddressBuffer : public Buffer
{
public:
	size_t GetBufferSize() const { return m_BufferSize; }

protected:
	ByteAddressBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource);
	ByteAddressBuffer(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc);

	virtual ~ByteAddressBuffer() = default;
private:
	size_t m_BufferSize;
};

