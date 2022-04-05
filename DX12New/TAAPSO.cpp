#include "TAAPSO.h"
#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

TAAPSO::TAAPSO(std::shared_ptr<Device> _device)
	:BasePSO(_device)
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\FullScreen_VS.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\TAA_PS.cso", &pixelShaderBlob));

	//设置根签名的标签,防止一些无必要的访问
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::MainPassCB].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	rootParameter[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	auto samplers = GetStaticSamplers();

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init_1_1(RootParameters::NumRootParameters, rootParameter, samplers.size(), samplers.data(), rootSignatureFlags);

	m_RootSignature = m_Device->CreateRootSignature(rsDesc.Desc_1_1);

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            RasterizerState;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
	} pipelineStateStream;

	//创建一个具有SRGB的颜色缓冲,为了gamma矫正
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 2;
	rtvFormats.RTFormats[0] = backBufferFormat;
	rtvFormats.RTFormats[1] = backBufferFormat;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);

	rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//设置PSO属性
	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.InputLayout = VertexPosition::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = { 1, 0 };
	pipelineStateStream.RasterizerState = rasterizerState;

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);
}

void TAAPSO::Apply(CommandList& _commandList)
{
	_commandList.SetPipelineState(m_PSO);
	_commandList.SetGraphicsRootSignature(m_RootSignature);

	//依次判断需要更新的属性,并绑定到渲染管线上

	if (m_DirtyFlags & DF_MainPassCB)
	{
		_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MainPassCB, *m_pAlignedMainPassCB);
	}

	if (m_DirtyFlags & DF_Material)
	{
		//设置贴图
		BindTexture(_commandList, 0, m_Textures[TAATexture::InputTexture], RootParameters::Textures);
		BindTexture(_commandList, 1, m_Textures[TAATexture::HistoryTexture], RootParameters::Textures);
	}

	//清空标志
	m_DirtyFlags = DF_None;
}
