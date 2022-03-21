#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(const std::wstring& _name /*= L""*/)
	:Buffer(_name), m_NumVertices(0), m_VertexStride(0), m_VBV({})
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateViews(size_t _numElements, size_t _elementSize)
{
	m_NumVertices = _numElements;
	m_VertexStride = _elementSize;

	m_VBV.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_VBV.SizeInBytes = static_cast<UINT>(m_NumVertices * m_VertexStride);
	m_VBV.StrideInBytes = static_cast<UINT>(m_VertexStride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc /* = nullptr */) const
{
	throw std::exception("VertexBuffer::GetShaderResourceView 不该被调用.");
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc /* = nullptr */) const
{
	throw std::exception("VertexBuffer::GetUnorderedAccessView 不该被调用.");
}
