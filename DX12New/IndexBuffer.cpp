#include "IndexBuffer.h"
#include <cassert>

IndexBuffer::IndexBuffer(const std::wstring& _name /*= L""*/)
	:Buffer(_name)
{
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::CreateViews(size_t _numElements, size_t _elementSize)
{
	assert(_elementSize == 2 || _elementSize == 4 && "索引必须为16位或32位整数");

	m_NumIndices = _numElements;
	m_IndexFormat = (_elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_IBV.BufferLocation = m_Resource->GetGPUVirtualAddress();
	m_IBV.SizeInBytes = static_cast<UINT>(_elementSize * _numElements);
	m_IBV.Format = m_IndexFormat;
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc /*= nullptr*/) const
{
	throw std::exception("IndexBuffer::GetShaderResourceView 不该被调用.");
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc /*= nullptr*/) const
{
	throw std::exception("IndexBuffer::GetUnorderedAccessView 不该被调用.");
}
