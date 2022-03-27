#include "GenerateMipsPSO.h"
#include "D3D12LibPCH.h"
#include "GanerateMips_GS.h"
#include "helpers.h"
#include "Device.h"
#include "PipelineStateObject.h"

GenerateMipsPSO::GenerateMipsPSO(Device& _device)
{
	auto device = _device.GetD3D12Device();

	//创建两个描述附表根参数
	//输入Mip
	CD3DX12_DESCRIPTOR_RANGE1 srcMip(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	//输出
	CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	//根参数（一个根常量，两个描述符表）
	CD3DX12_ROOT_PARAMETER1 rootParameters[GenerateMips::NumRootParameters];
	rootParameters[GenerateMips::GenerateMipsCB].InitAsConstants(sizeof(GenerateMipsCB) / 4, 0);
	rootParameters[GenerateMips::SrcMip].InitAsDescriptorTable(1, &srcMip);
	rootParameters[GenerateMips::OutMip].InitAsDescriptorTable(1, &outMip);

	//静态采样器
	CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

	//根签名描述
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(GenerateMips::NumRootParameters, rootParameters, 1, &linearClampSampler);

	m_RootSiganture = _device.CreateRootSignature(rootSignatureDesc.Desc_1_1);

	//创建PSO
	struct PipelineStateStream 
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
	}pipelineStateStream;

	//设置根签名和着色器
	pipelineStateStream.pRootSignature = m_RootSiganture->GetRootSignature().Get();
	pipelineStateStream.CS = { g_GanerateMips_GS, sizeof(g_GanerateMips_GS) };

	m_PSO = _device.CreatePipelineStateObject(pipelineStateStream);

	//分配描述符
	m_DefaultUAV = _device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);

	//创建UAV
	for (UINT i = 0; i < 4; ++i)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Texture2D.MipSlice = 1;
		desc.Texture2D.PlaneSlice = 0;

		device->CreateUnorderedAccessView(nullptr, nullptr, &desc, m_DefaultUAV.GetDescriptorHandle(i));
	}
}
