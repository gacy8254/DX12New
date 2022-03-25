#pragma once
#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include <cstdint>

struct PanoToCubemapCB
{
	//��ǰcubemapÿ����ĳߴ磬��ǰ��mip�ȼ���
	uint32_t CubemapSize;
	//��һ�����ɵ�mipmap�ȼ�
	uint32_t FirstMip;
	//���ɵ�������ƨ����
	uint32_t NumMips;
};

namespace PanoToCubemapRS
{
	enum {
		PanoToCubemapCB,
		SrcTexture,
		DstMips,
		NumRootParameters,
	};
}


class PanoToCubemapPSO
{
public:
	PanoToCubemapPSO();

	const dx12lib::RootSignature& GetRootSignature() const { return m_RootSignature; }

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO() const { return m_PSO; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const { return m_DefaultUAV.GetDescriptorHandle(); }

private:
	dx12lib::RootSignature m_RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	DescriptorAllocation m_DefaultUAV;

};

