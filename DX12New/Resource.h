#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <string>
#include <memory>


class Resource
{
public:
	Resource(const std::wstring& _name = L"");
	Resource(const D3D12_RESOURCE_DESC _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr, const std::wstring& _name = L"");
	Resource(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const std::wstring& _name = L"");
	Resource(const Resource& _copy);
	Resource(Resource&& _copy);

	Resource& operator=(const Resource& _other);
	Resource& operator=(Resource&& _other);

	virtual ~Resource();

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

	//重置资源
	//应当只被CommandList类调用 
	virtual void SetResource(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr);

	//获取资源的SRV
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const = 0;
	//获取资源的UAV
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const = 0;

	void SetName(const std::wstring& _name);

	//重置底层资源
	//这在更改交换链大小时很有用
	virtual void Reset();

protected:

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
	std::unique_ptr<D3D12_CLEAR_VALUE> m_ClearValue = nullptr;
	std::wstring m_Name;
};

