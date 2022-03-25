#pragma once

#include "Buffer.h"

#include <d3d12.h> // For ID3D12Resource
#include <wrl/client.h> // For ComPtr

class ConstantBuffer : public Buffer
{
public:
	size_t GetSizeInBytes() const { return m_SizeInBytes; }

protected:
	ConstantBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource);
	virtual ~ConstantBuffer();

private:
	size_t m_SizeInBytes;
};

