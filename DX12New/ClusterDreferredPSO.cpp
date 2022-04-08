#include "ClusterDreferredPSO.h"

#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;


ClusterDreferredPSO::ClusterDreferredPSO(std::shared_ptr<Device> _device, UINT _elementNum, UINT _elementSize)
	:BasePSO(_device)
{
	ComPtr<ID3DBlob> computerShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\ClusterDrferred_CS.cso", &computerShaderBlob));

	//输出
	CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	//根参数
	CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameter::NumRootParameters];
	rootParameters[RootParameter::LightProperites].InitAsConstants(sizeof(LightProperties) / 4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[RootParameter::MainPassCB].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[RootParameter::LightList].InitAsDescriptorTable(1, &outMip, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[RootParameter::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[RootParameter::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init_1_1(RootParameter::NumRootParameters, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignature = m_Device->CreateRootSignature(rsDesc.Desc_1_1);

	//创建PSO
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
	}pipelineStateStream;

	//设置根签名和着色器
	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computerShaderBlob.Get());

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.NumElements = _elementNum;
	uavDesc.Buffer.StructureByteStride = _elementSize;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	m_UAV = m_Device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc);
}

void ClusterDreferredPSO::Apply(CommandList& _commandList)
{
	_commandList.SetComputerRootSignature(m_RootSignature);
	_commandList.SetPipelineState(m_PSO);

	_commandList.SetUnorderedAccessView(RootParameter::LightList, 0, m_UAV);

	if (m_DirtyFlags & DF_MainPassCB)
	{
		_commandList.SetComputerDynamicConstantBuffer(RootParameter::MainPassCB, *m_pAlignedMainPassCB);
	}

	if (m_DirtyFlags & DF_PointLights)
	{
		_commandList.SetComputerDynamicStructuredBuffer(RootParameter::PointLights, m_PointLights);
	}

	if (m_DirtyFlags & DF_SpotLights)
	{
		_commandList.SetComputerDynamicStructuredBuffer(RootParameter::SpotLights, m_SpotLights);
	}

	if (m_DirtyFlags & (DF_PointLights | DF_SpotLights | DF_DirectionalLights))
	{
		LightProperties LightProps;
		LightProps.NumPointLights = static_cast<uint32_t>(m_PointLights.size());
		LightProps.NumSpotLights = static_cast<uint32_t>(m_SpotLights.size());
		LightProps.NumDirectionalLights = 0;

		_commandList.SetComputer32BitConstants(RootParameter::LightProperites, LightProps);
	}

	//清空标志
	m_DirtyFlags = DF_None;
}
