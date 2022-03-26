#pragma once
#include "d3dx12.h"

#include <wrl.h>
#include <vector>
#include <memory>

class Device;

class RootSignature
{
public:



	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_RootSignature; }

	const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const { return m_RootSignatureDesc; }

	//获取描述符表的掩码值
	uint32_t GteDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE _type) const;
	//获取描述符表中的描述符数量
	uint32_t GetNumDescriptors(uint32_t _rootIndex) const;

protected:
	//暂时不知道是干啥的
	friend class std::default_delete<RootSignature>;

	RootSignature(Device& _device, const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc);

	virtual ~RootSignature();
private:
	void Destroy();
	void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc);

	Device& m_Device;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC1 m_RootSignatureDesc;

	//每个描述符表中描述符的数量
	//最多支持32个描述符表，因为根签名使用32位掩码表示描述符表
	uint32_t m_NumDescriptorPerTable[32];


	//表示根参数索引的位掩码
	uint32_t m_SmaplerTableBitMask = 0;
	uint32_t m_DescriptorTableBitMask = 0;

	
};

