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

class PipelineStateObject;
class Device;

class PanoToCubemapPSO
{
public:
	PanoToCubemapPSO(Device& _device);

	std::shared_ptr<RootSignature> GetRootSignature() const { return m_RootSignature; }

	std::shared_ptr<PipelineStateObject> GetPSO() const { return m_PSO; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const { return m_DefaultUAV.GetDescriptorHandle(); }

private:
	std::shared_ptr<RootSignature> m_RootSignature;
	std::shared_ptr<PipelineStateObject> m_PSO;

	DescriptorAllocation m_DefaultUAV;

};

