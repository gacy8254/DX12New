#include "StructuredBuffer.h"

StructuredBuffer::StructuredBuffer(Device& _device, size_t _numElements, size_t _elementSize)
	:Buffer(_device, CD3DX12_RESOURCE_DESC::Buffer(_numElements * _elementSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)),
	m_ElementSize(_elementSize), m_NumElements(_numElements)
{
	m_CounterBuffer = m_Device.CreateByteAddressBuffer(4);
}

StructuredBuffer::StructuredBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numElements, size_t _elementSize)
	:Buffer(_device, _resource), m_ElementSize(_elementSize), m_NumElements(_numElements)
{
	m_CounterBuffer = m_Device.CreateByteAddressBuffer(4);
}