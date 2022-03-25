#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <string>
#include <memory>
#include "Device.h"


class Resource
{
public:	
	Device& GetDevice() const { return m_Device; }

	//检查资源是否有效
	bool IsVaild() const { return (m_Resource != nullptr); }

	//获取资源
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const { return m_Resource; }

	//获取资源描述
	D3D12_RESOURCE_DESC GetResourceDesc() const {
		D3D12_RESOURCE_DESC desc = {};
		if (m_Resource)
		{
			desc = m_Resource->GetDesc();
		}
		return desc;
	}

	void SetName(const std::wstring& _name);
	const std::wstring GetName() { return m_Name; }

	//检查格式支持
	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;

	
protected:
	Resource(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr);
	Resource(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr);

	virtual ~Resource() = default;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
	std::wstring m_Name;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport;
	std::unique_ptr<D3D12_CLEAR_VALUE> m_ClearValue;

	Device& m_Device;

private:
	void CheckFeatureSupport();

};

