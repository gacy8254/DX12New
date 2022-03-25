#include "ByteAddressBuffer.h"
#include "D3D12LibPCH.h"
#include "Application.h"

ByteAddressBuffer::ByteAddressBuffer(const std::wstring& name /*= L""*/)
	:Buffer(name)
{
	m_SRV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_UAV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ByteAddressBuffer::ByteAddressBuffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSize, const std::wstring& name /*= L""*/)
	:Buffer(_resDesc, _numElements, _elementSize, name)
{}

void ByteAddressBuffer::CreateViews(size_t _numElements, size_t _elementSize)
{
	auto device = Application::Get().GetDevice();

	m_BufferSize = Math::AlignUp(_numElements * _elementSize, 4);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_SRV.GetDescriptorHandle());

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, m_UAV.GetDescriptorHandle());
}
