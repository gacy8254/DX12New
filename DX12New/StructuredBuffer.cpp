#include "StructuredBuffer.h"
#include "Application.h"

StructuredBuffer::StructuredBuffer(const std::wstring& name /*= L""*/)
	:Buffer(name),
	m_CounterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 1, 4, name + L"Counter"),
	m_ElementSize(0),
	m_NumElements(0)
{
	m_SRV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_UAV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

StructuredBuffer::StructuredBuffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& name /*= L""*/)
	:Buffer(_resDesc, _numElements, _elementSize, name),
	m_CounterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 1, 4, name + L"Counter"),
	m_ElementSize(_elementSize),
	m_NumElements(_numElements)
{
	m_SRV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_UAV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void StructuredBuffer::CreateViews(size_t _numElements, size_t _elementSize)
{
	auto device = Application::Get().GetDevice();

	m_NumElements = _numElements;
	m_ElementSize = _elementSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_NumElements);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.StructureByteStride = static_cast<UINT>(m_ElementSize);

	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_SRV.GetDescriptorHandle());

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.NumElements = static_cast<UINT>(m_NumElements);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.StructureByteStride = static_cast<UINT>(m_ElementSize);

	device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, m_UAV.GetDescriptorHandle());
}
