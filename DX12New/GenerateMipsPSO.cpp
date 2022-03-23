#include "GenerateMipsPSO.h"
#include "D3D12LibPCH.h"
#include "GanerateMips_GS.h"
#include "Application.h"
#include "helpers.h"


GenerateMipsPSO::GenerateMipsPSO()
{
	auto device = Application::Get().GetDevice();

	//��ȡ֧�ֵ���߰汾
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//���������������������
	//����Mip
	CD3DX12_DESCRIPTOR_RANGE1 srcMip(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	//���
	CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	//��������һ����������������������
	CD3DX12_ROOT_PARAMETER1 rootParameters[GenerateMips::NumRootParameters];
	rootParameters[GenerateMips::GenerateMipsCB].InitAsConstants(sizeof(GenerateMipsCB) / 4, 0);
	rootParameters[GenerateMips::SrcMip].InitAsDescriptorTable(1, &srcMip);
	rootParameters[GenerateMips::OutMip].InitAsDescriptorTable(1, &outMip);

	//��̬������
	CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
		);

	//��ǩ������
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(GenerateMips::NumRootParameters, rootParameters, 1, &linearClampSampler);

	m_RootSiganture.SetRootSignatureDesc(rootSignatureDesc.Desc_1_1, featureData.HighestVersion);

	//����PSO
	struct PipelineStateStream 
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
	}pipelineStateStream;

	//���ø�ǩ������ɫ��
	pipelineStateStream.pRootSignature = m_RootSiganture.GetRootSignature().Get();
	pipelineStateStream.CS = { g_GanerateMips_GS, sizeof(g_GanerateMips_GS) };

	D3D12_PIPELINE_STATE_STREAM_DESC PSOStreamDesc = { sizeof(pipelineStateStream), &pipelineStateStream };

	ThrowIfFailed(device->CreatePipelineState(&PSOStreamDesc, IID_PPV_ARGS(&m_PSO)));

	//����������
	m_DefaultUAV = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);

	//����UAV
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
