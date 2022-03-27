#include "Resource.h"
#include "D3D12LibPCH.h"
#include "ResourceStateTracker.h"

Resource::Resource(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
	: m_Device(_device)
{
	auto device = m_Device.GetD3D12Device();

	if (_clearValue)
	{
		m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_clearValue);
	}

	auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&p,
		D3D12_HEAP_FLAG_NONE,
		&_resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		m_ClearValue.get(),
		IID_PPV_ARGS(&m_Resource)));

	ResourceStateTracker::AddGlobalResourceState(m_Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

	CheckFeatureSupport();
}

Resource::Resource(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
	:m_Device(_device), m_Resource(_resource)
{
	if (_clearValue)
	{
		m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_clearValue);
	}
	CheckFeatureSupport();
}

void Resource::SetName(const std::wstring& _name)
{
	m_Name = _name;
	if (m_Resource && !m_Name.empty())
	{
		m_Resource->SetName(m_Name.c_str());
	}
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
	return(m_FormatSupport.Support1 & formatSupport) != 0;
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
	return(m_FormatSupport.Support2 & formatSupport) != 0;
}

void Resource::CheckFeatureSupport()
{
	if (m_Resource)
	{
		auto desc = m_Resource->GetDesc();
		auto device = m_Device.GetD3D12Device();

		m_FormatSupport.Format = desc.Format;
		ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &m_FormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
	}
	else
	{
		m_FormatSupport = {};
	}
}
