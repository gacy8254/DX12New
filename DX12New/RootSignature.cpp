#include "RootSignature.h"
#include "D3D12LibPCH.h"
#include "Application.h"
#include "Device.h"

uint32_t RootSignature::GteDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE _type) const
{
	uint32_t bitMask = 0;
	switch (_type)
	{
	case  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		bitMask = m_DescriptorTableBitMask;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		bitMask = m_SmaplerTableBitMask;
	}

	return bitMask;
}

uint32_t RootSignature::GetNumDescriptors(uint32_t _rootIndex) const
{
	assert(_rootIndex < 32);
	return m_NumDescriptorPerTable[_rootIndex];
}

RootSignature::~RootSignature()
{
	Destroy();
}

void RootSignature::Destroy()
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

	m_SmaplerTableBitMask = 0;
	m_DescriptorTableBitMask = 0;

	memset(m_NumDescriptorPerTable, 0, sizeof(m_NumDescriptorPerTable));
}

void RootSignature::SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc)
{
	//确保先前分配的根签名描述已经被清理
	Destroy();

	auto device = m_Device.GetD3D12Device();

	//根参数的数量
	UINT numParameters = _rootSignatureDesc.NumParameters;
	D3D12_ROOT_PARAMETER1* parameters = numParameters > 0 ? new D3D12_ROOT_PARAMETER1[numParameters] : nullptr;

	//遍历所有的根参数
	for (UINT i = 0; i < numParameters; i++)
	{
		const D3D12_ROOT_PARAMETER1& rootParamater = _rootSignatureDesc.pParameters[i];
		parameters[i] = rootParamater;

		//判断参数类型是描述符表
		if (rootParamater.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			//获取表中描述符的数量
			UINT numDescriptorRanges = rootParamater.DescriptorTable.NumDescriptorRanges;
			D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges = numDescriptorRanges > 0 ? new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges] : nullptr;

			//从传递过来的参数中拷贝数值
			memcpy(pDescriptorRanges, rootParamater.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

			//设置根参数中的描述符表
			parameters[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
			parameters[i].DescriptorTable.pDescriptorRanges = pDescriptorRanges;

			//根据描述符表的类型设置掩码值
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

			//计算描述符表中有多少个描述符
			for (UINT j = 0; j < numDescriptorRanges; ++j)
			{
				m_NumDescriptorPerTable[i] += pDescriptorRanges[j].NumDescriptors;
			}
		}
	}
	//设置根签名描述
	m_RootSignatureDesc.NumParameters = numParameters;
	m_RootSignatureDesc.pParameters = parameters;

	//获取静态描述符的数量
	UINT numStaticSamplers = _rootSignatureDesc.NumStaticSamplers;
	D3D12_STATIC_SAMPLER_DESC* pStaticSampler = numStaticSamplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers] : nullptr;
	//拷贝静态描述符的内容
	if (pStaticSampler)
	{
		memcpy(pStaticSampler, _rootSignatureDesc.pStaticSamplers, sizeof(D3D12_STATIC_SAMPLER_DESC) * numStaticSamplers);
	}

	//设置根签名描述
	m_RootSignatureDesc.NumStaticSamplers = numStaticSamplers;
	m_RootSignatureDesc.pStaticSamplers = pStaticSampler;

	//设置根签名标志
	D3D12_ROOT_SIGNATURE_FLAGS flasg = _rootSignatureDesc.Flags;
	m_RootSignatureDesc.Flags = flasg;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc;
	versionRootSignatureDesc.Init_1_1(numParameters, parameters, numStaticSamplers, pStaticSampler, flasg);

	//根签名序列化
	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	D3D_ROOT_SIGNATURE_VERSION highestVersion = m_Device.GetHighestRootSignatureVersion();

	//序列化
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&versionRootSignatureDesc, highestVersion, &rootSignatureBlob, &errorBlob));

	//创建根签名
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

}

RootSignature::RootSignature(Device& _device, const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc)
	:m_Device(_device),
	m_RootSignatureDesc{},
	m_NumDescriptorPerTable{0},
	m_SmaplerTableBitMask(0),
	m_DescriptorTableBitMask(0)
{
	SetRootSignatureDesc(_rootSignatureDesc);
}
