#include "RootSignature.h"
#include "D3D12LibPCH.h"

uint32_t dx12lib::RootSignature::GteDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE _type) const
{
	uint32_t bitMask = 0;
	switch (_type)
	{
	case  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		bitMask = m_DescriptorTableBitMask;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		bitMask = m_SmaplerTableBitMask;
	}

	return bitMask;
}

uint32_t dx12lib::RootSignature::GetNumDescriptors(uint32_t _rootIndex) const
{
	assert(_rootIndex < 32);
	return m_NumDescriptorPerTable[_rootIndex];
}

dx12lib::RootSignature::RootSignature(Microsoft::WRL::ComPtr<ID3D12Device2> _device, const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc)
	:m_Device(_device), m_RootSignatureDesc{}
{
	SetRootSignatureDesc(_rootSignatureDesc);
}

dx12lib::RootSignature::RootSignature()
{

}

dx12lib::RootSignature::~RootSignature()
{

}

void dx12lib::RootSignature::Destroy()
{
	for (UINT i = 0; i < m_RootSignatureDesc.NumParameters; i++)
	{
		const D3D12_ROOT_PARAMETER1& rootParameter = m_RootSignatureDesc.pParameters[i];
		if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			delete[] rootParameter.DescriptorTable.pDescriptorRanges;
		}
	}

	delete[] m_RootSignatureDesc.pParameters;
	m_RootSignatureDesc.pParameters = nullptr;
	m_RootSignatureDesc.NumParameters = 0;

	delete[] m_RootSignatureDesc.pStaticSamplers;
	m_RootSignatureDesc.pStaticSamplers = nullptr;
	m_RootSignatureDesc.NumStaticSamplers = 0;

	memset(m_NumDescriptorPerTable, 0, sizeof(m_NumDescriptorPerTable));
}

void dx12lib::RootSignature::SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc)
{
	//ȷ����ǰ����ĸ�ǩ�������Ѿ�������
	Destroy();

	//������������
	UINT numParameters = _rootSignatureDesc.NumParameters;
	D3D12_ROOT_PARAMETER1* parameters = numParameters > 0 ? new D3D12_ROOT_PARAMETER1[numParameters] : nullptr;

	//�������еĸ�����
	for (UINT i = 0; i < numParameters; i++)
	{
		const D3D12_ROOT_PARAMETER1& rootParamater = _rootSignatureDesc.pParameters[i];
		parameters[i] = rootParamater;

		//�жϲ�����������������
		if (rootParamater.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			//��ȡ����������������
			UINT numDescriptorRanges = rootParamater.DescriptorTable.NumDescriptorRanges;
			D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges = numDescriptorRanges > 0 ? new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges] : nullptr;

			//�Ӵ��ݹ����Ĳ����п�����ֵ
			memcpy(pDescriptorRanges, rootParamater.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

			//���ø������е���������
			parameters[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
			parameters[i].DescriptorTable.pDescriptorRanges = pDescriptorRanges;

			//�������������������������ֵ
			if (numDescriptorRanges > 0)
			{
				switch (pDescriptorRanges[0].RangeType)
				{
				case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				case  D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
					m_DescriptorTableBitMask |= (1 << i);
					break;
				case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
					m_SmaplerTableBitMask |= (1 << i);
					break;
				}
			}

			//���������������ж��ٸ�������
			for (UINT j = 0; j < numDescriptorRanges; ++j)
			{
				m_NumDescriptorPerTable[i] += pDescriptorRanges[j].NumDescriptors;
			}
		}
	}
	//���ø�ǩ������
	m_RootSignatureDesc.NumParameters = numParameters;
	m_RootSignatureDesc.pParameters = parameters;

	//��ȡ��̬������������
	UINT numStaticSamplers = _rootSignatureDesc.NumStaticSamplers;
	D3D12_STATIC_SAMPLER_DESC* pStaticSampler = numStaticSamplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers] : nullptr;
	//������̬������������
	if (pStaticSampler)
	{
		memcpy(pStaticSampler, _rootSignatureDesc.pStaticSamplers, sizeof(D3D12_STATIC_SAMPLER_DESC) * numStaticSamplers);
	}

	//���ø�ǩ������
	m_RootSignatureDesc.NumStaticSamplers = numStaticSamplers;
	m_RootSignatureDesc.pStaticSamplers = pStaticSampler;

	//���ø�ǩ����־
	D3D12_ROOT_SIGNATURE_FLAGS flasg = _rootSignatureDesc.Flags;
	m_RootSignatureDesc.Flags = flasg;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc;
	versionRootSignatureDesc.Init_1_1(numParameters, parameters, numStaticSamplers, pStaticSampler, flasg);

	//���֧�ֵĸ�ǩ���汾
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//��ǩ�����л�
	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	//���л�
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&versionRootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	//������ǩ��
	ThrowIfFailed(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

}
