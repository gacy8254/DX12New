#pragma once

#include "Device.h"
#include "Resource.h"
#include "DescriptorAllocation.h"

#include <d3d12.h>
#include <memory>

class UnorderedAccessView
{
public:
	std::shared_ptr<Resource> GetResource() const { return m_Resource; }

	std::shared_ptr<Resource> GetCounterResource() const { return m_CounterResource; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return m_Descriptor.GetDescriptorHandle(); }

protected:
	UnorderedAccessView(Device& _device, 
		const std::shared_ptr<Resource>& _resource, 
		const std::shared_ptr<Resource>& _counterResource = nullptr, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav = nullptr);

	virtual ~UnorderedAccessView() = default;

private:
	Device& m_Device;
	std::shared_ptr<Resource> m_Resource;
	std::shared_ptr<Resource> m_CounterResource;
	DescriptorAllocation      m_Descriptor;

};
