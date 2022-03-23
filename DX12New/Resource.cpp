#include "Resource.h"
#include "D3D12LibPCH.h"
#include "Application.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& _name /*= L""*/)
	:m_Name(_name), m_FormatSupport({})
{
	CheckFeatureSupport();
}

Resource::Resource(const D3D12_RESOURCE_DESC _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/, const std::wstring& _name /*= L""*/)
{
	auto device = Application::Get().GetDevice();

	if (_clearValue)
	{
		m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_clearValue);
	}

	//创建资源
	auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&p,
		D3D12_HEAP_FLAG_NONE,
		&_resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		m_ClearValue.get(),
		IID_PPV_ARGS(&m_Resource)));

	//追踪资源状态
	ResourceStateTracker::AddGlobalResourceState(m_Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

	//检查格式支持
	CheckFeatureSupport();

	SetName(_name);
}

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const std::wstring& _name /*= L""*/)
	:m_Resource(_resource), m_FormatSupport({})
{
	CheckFeatureSupport();
	SetName(_name);
}

Resource::Resource(const Resource& _copy)
	: m_Resource(_copy.m_Resource), m_Name(_copy.m_Name), m_ClearValue(std::make_unique<D3D12_CLEAR_VALUE>(*_copy.m_ClearValue)), m_FormatSupport(_copy.m_FormatSupport)
{
}

Resource::~Resource()
{
}

void Resource::SetName(const std::wstring& _name)
{
	m_Name = _name;
	if (m_Resource && !m_Name.empty())
	{
		m_Resource->SetName(m_Name.c_str());
	}
}

void Resource::Reset()
{
	m_Resource.Reset();
	m_ClearValue.reset();
	m_FormatSupport = {};
	m_Name.clear();
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
		auto device = Application::Get().GetDevice();

		m_FormatSupport.Format = desc.Format;
		ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &m_FormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
	}
	else
	{
		m_FormatSupport = {};
	}
}

Resource& Resource::operator=(const Resource& _other)
{
	if (this != &_other)
	{
		m_Resource = _other.m_Resource;
		m_Name = _other.m_Name;
		m_FormatSupport = _other.m_FormatSupport;
		if (_other.m_ClearValue)
		{
			m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_other.m_ClearValue);
		}
	}

	return *this;
}

Resource& Resource::operator=(Resource&& _other)
{
	if (this != &_other)
	{
		m_Resource = std::move(_other.m_Resource);
		m_FormatSupport = _other.m_FormatSupport;
		m_Name = std::move(_other.m_Name);
		m_ClearValue = std::move(_other.m_ClearValue);

		_other.Reset();
	}

	return *this;
}

Resource::Resource(Resource&& _copy)
	: m_Resource(std::move(_copy.m_Resource)), m_Name(std::move(_copy.m_Name)), m_ClearValue(std::move(_copy.m_ClearValue)), m_FormatSupport(_copy.m_FormatSupport)
{
}

void Resource::SetResource(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
{
	m_Resource = _resource;
	if (m_ClearValue)
	{
		m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*_clearValue);
	}
	else
	{
		m_ClearValue.reset();
	}
	CheckFeatureSupport();
	SetName(m_Name);
}
