#include "Resource.h"
#include "D3D12LibPCH.h"
#include "Application.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& _name /*= L""*/)
	:m_Name(_name)
{}

Resource::Resource(const D3D12_RESOURCE_DESC _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/, const std::wstring& _name /*= L""*/)
{
	auto device = Application::Get().GetDevice();

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

	SetName(_name);
}

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const std::wstring& _name /*= L""*/)
	:m_Resource(_resource)
{
	SetName(_name);
}

Resource::Resource(const Resource& _copy)
	: m_Resource(_copy.m_Resource), m_Name(_copy.m_Name), m_ClearValue(std::make_unique<D3D12_CLEAR_VALUE>(*_copy.m_ClearValue))
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
}

Resource& Resource::operator=(const Resource& _other)
{
	if (this != &_other)
	{
		m_Resource = _other.m_Resource;
		m_Name = _other.m_Name;
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
		m_Resource = _other.m_Resource;
		m_Name = _other.m_Name;
		m_ClearValue = std::move(_other.m_ClearValue);

		_other.m_Resource.Reset();
		_other.m_Name.clear();
	}

	return *this;
}

Resource::Resource(Resource&& _copy)
	: m_Resource(std::move(_copy.m_Resource)), m_Name(std::move(_copy.m_Name)), m_ClearValue(std::move(_copy.m_ClearValue))
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
	SetName(m_Name);
}
