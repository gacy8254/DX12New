#pragma once
#include "d3dx12.h"

#include <wrl.h>
#include <vector>
#include <memory>

namespace dx12lib
{
	class RootSignature
	{
	public:
		RootSignature();
		RootSignature(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion);

		virtual ~RootSignature();
		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_RootSignature; }

		const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const { return m_RootSignatureDesc; }

		//��ȡ�������������ֵ
		uint32_t GteDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE _type) const;
		//��ȡ���������е�����������
		uint32_t GetNumDescriptors(uint32_t _rootIndex) const;

		void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion);
	protected:
		//��ʱ��֪���Ǹ�ɶ��
		friend class std::default_delete<RootSignature>;

	private:
		void Destroy();

		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
		D3D12_ROOT_SIGNATURE_DESC1 m_RootSignatureDesc;

		//ÿ������������������������
		//���֧��32������������Ϊ��ǩ��ʹ��32λ�����ʾ��������
		uint32_t m_NumDescriptorPerTable[32];


		//��ʾ������������λ����
		uint32_t m_SmaplerTableBitMask = 0;
		uint32_t m_DescriptorTableBitMask = 0;

		
	};
}
