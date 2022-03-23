#pragma once

#include "DescriptorAllocation.h"
#include "RootSignature.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

struct alignas(16) GenerateMipsCB 
{
	uint32_t SrcMipLevel;			//Դ��ͼMip�ȼ�
	uint32_t NumMipLevels;			//Ҫ���ɵ�Mip����
	uint32_t SrcDimension;			//Դ��ͼ�Ŀ��
	uint32_t IsSRGB;				//�ǲ���SRGB
	DirectX::XMFLOAT2 TexelSize;	//1.0 / outMip1.Dimensions
};

namespace GenerateMips
{
	enum 
	{
		GenerateMipsCB,
		SrcMip,
		OutMip,
		NumRootParameters
	};
}

class GenerateMipsPSO
{
public:
	GenerateMipsPSO();

	const dx12lib::RootSignature& GetRootSignature() const { return m_RootSiganture; }

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO() const { return m_PSO; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const { return m_DefaultUAV.GetDescriptorHandle(); }

private:
	dx12lib::RootSignature m_RootSiganture;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	//�����������ĸ�Mips��������ɫ���е�һЩUAV��û�а󶨵�������
	//����ʼ��Ϊ��ɫ���е����в����ṩ��Ч������������ʹ������û�б�ʹ��
	//m_DefaultUAV�����ṩĬ�ϵĿ�UAV
	DescriptorAllocation m_DefaultUAV;
};

