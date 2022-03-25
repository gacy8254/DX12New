#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numVertices, size_t _vertexStride)
	:Buffer(_device, _resource),
	m_NumVertices(_numVertices), m_VertexStride(_vertexStride), m_VBV{}
{
	CreateVertexBufferViews();
}

VertexBuffer::VertexBuffer(Device& _device, size_t _numVertices, size_t _vertexStride)
	:Buffer(_device, CD3DX12_RESOURCE_DESC::Buffer(_numVertices* _vertexStride)),
	m_NumVertices(_numVertices), m_VertexStride(_vertexStride), m_VBV{}
{
	CreateVertexBufferViews();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateVertexBufferViews()
{
	m_VBV.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_VBV.SizeInBytes = static_cast<UINT>(m_NumVertices * m_VertexStride);
	m_VBV.StrideInBytes = static_cast<UINT>(m_VertexStride);
}