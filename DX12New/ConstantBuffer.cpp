#include "ConstantBuffer.h"

ConstantBuffer::ConstantBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource)
	:Buffer(_device, _resource)
{
	m_SizeInBytes = GetResourceDesc().Width;
}

ConstantBuffer::~ConstantBuffer()
{}
