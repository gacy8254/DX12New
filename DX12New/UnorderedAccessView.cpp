#include "UnorderedAccessView.h"
#include "D3D12LibPCH.h"

class MakeResource : public Resource
{
public:
	MakeResource(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr)
		:Resource(_device, _resource, _clearValue){}
	MakeResource(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr)
		:Resource(_device, _resourceDesc, _clearValue){}
	virtual ~MakeResource() = default;
};

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
	else
	{
		auto d3d12Device = m_Device.GetD3D12Device();

		auto om = CD3DX12_RESOURCE_DESC::Buffer(_uav->Buffer.NumElements * _uav->Buffer.StructureByteStride, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto resource = std::make_shared<MakeResource>(m_Device, om);
		m_Resource = resource;
		//ThrowIfFailed(d3d12Device->CreateCommittedResource(
		//	&mo,
		//	D3D12_HEAP_FLAG_NONE,
		//	&om,
		//	//&CD3DX12_RESOURCE_DESC::Buffer(1024, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(&m_Resource->GetResource())));
		d3d12Resource = resource->GetResource();
	}

	m_Descriptor = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateUnorderedAccessView(d3d12Resource.Get(), d3d12CounterResource.Get(), _uav, m_Descriptor.GetDescriptorHandle());
}
