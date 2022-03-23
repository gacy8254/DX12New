#pragma once

#include "DescriptorAllocation.h"
#include "RootSignature.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

struct alignas(16) GenerateMipsCB 
{
	uint32_t SrcMipLevel;			//源贴图Mip等级
	uint32_t NumMipLevels;			//要生成的Mip数量
	uint32_t SrcDimension;			//源贴图的宽高
	uint32_t IsSRGB;				//是不是SRGB
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

	//当生成少于四个Mips，计算着色器中的一些UAV将没有绑定到描述符
	//建议始终为着色器中的所有参数提供有效的描述符，即使描述符没有被使用
	//m_DefaultUAV用于提供默认的空UAV
	DescriptorAllocation m_DefaultUAV;
};

