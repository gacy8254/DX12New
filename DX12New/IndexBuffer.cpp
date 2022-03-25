#include "IndexBuffer.h"
#include <cassert>

IndexBuffer::IndexBuffer(Device& _device, size_t _numIndicies, DXGI_FORMAT _format)
	:Buffer(_device, CD3DX12_RESOURCE_DESC::Buffer(_numIndicies * (_format == DXGI_FORMAT_R16_UINT ? 2 : 4))),
	m_NumIndices(_numIndicies), m_IndexFormat(_format), m_IBV{}
{
	assert(_format == DXGI_FORMAT_R16_UINT || _format == DXGI_FORMAT_R32_UINT);
	CreateIndexBufferViews();
}

IndexBuffer::IndexBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numIndicies, DXGI_FORMAT _format)
	:Buffer(_device, _resource), m_NumIndices(_numIndicies), m_IndexFormat(_format), m_IBV{}
{
	assert(_format == DXGI_FORMAT_R16_UINT || _format == DXGI_FORMAT_R32_UINT);
	CreateIndexBufferViews();
}

void IndexBuffer::CreateIndexBufferViews()
{
	UINT bufferSize = m_NumIndices * (m_IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);

	m_IBV.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_IBV.SizeInBytes = bufferSize;
	m_IBV.Format = m_IndexFormat;
}