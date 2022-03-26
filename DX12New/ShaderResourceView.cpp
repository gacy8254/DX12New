#include "ShaderResourceView.h"
#include <assert.h>

ShaderResourceView::ShaderResourceView(Device& _device, const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv /*= nullptr*/)
	:m_Device(_device), m_Resource(_resource)
{
	assert(_resource || _srv);

	auto d3d12Resource = m_Resource ? m_Resource->GetResource() : nullptr;
	auto device = m_Device.GetD3D12Device();

	m_Descriptor = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateShaderResourceView(d3d12Resource.Get(), _srv, m_Descriptor.GetDescriptorHandle());
}
