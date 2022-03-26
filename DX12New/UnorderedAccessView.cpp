#include "UnorderedAccessView.h"
#include "D3D12LibPCH.h"

UnorderedAccessView::UnorderedAccessView(Device& _device, 
	const std::shared_ptr<Resource>& _resource, 
	const std::shared_ptr<Resource>& _counterResource /*= nullptr*/, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav /*= nullptr*/)
	:m_Device(_device), m_Resource(_resource), m_CounterResource(_counterResource)
{
	assert(_resource || _uav);

	auto d3d12Resource = m_Resource ? m_Resource->GetResource() : nullptr;
	auto device = m_Device.GetD3D12Device();
	auto d3d12CounterResource = m_CounterResource ? m_CounterResource->GetResource() : nullptr;

	if (m_Resource)
	{
		auto d3d12ResourceDesc = m_Resource->GetResourceDesc();

		assert((d3d12ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
	}

	m_Descriptor = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateUnorderedAccessView(d3d12Resource.Get(), d3d12CounterResource.Get(), _uav, m_Descriptor.GetDescriptorHandle());
}
