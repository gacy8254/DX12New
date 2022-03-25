#pragma once
#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include <cstdint>

struct PanoToCubemapCB
{
	//当前cubemap每个面的尺寸，当前的mip等级下
	uint32_t CubemapSize;
	//第一个生成的mipmap等级
	uint32_t FirstMip;
	//生成的米拍马屁数量
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

