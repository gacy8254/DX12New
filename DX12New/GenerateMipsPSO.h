#pragma once

#include "DescriptorAllocation.h"
#include "RootSignature.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

class Device;
class PipelineStateObject;

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
	GenerateMipsPSO(Device& _device);

	const std::shared_ptr<RootSignature> GetRootSignature() const { return m_RootSiganture; }

	std::shared_ptr<PipelineStateObject> GetPSO() const { return m_PSO; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const { return m_DefaultUAV.GetDescriptorHandle(); }

private:
	std::shared_ptr<RootSignature> m_RootSiganture;
	std::shared_ptr<PipelineStateObject> m_PSO;

	//�����������ĸ�Mips��������ɫ���е�һЩUAV��û�а󶨵�������
	//����ʼ��Ϊ��ɫ���е����в����ṩ��Ч������������ʹ������û�б�ʹ��
	//m_DefaultUAV�����ṩĬ�ϵĿ�UAV
	DescriptorAllocation m_DefaultUAV;
};

