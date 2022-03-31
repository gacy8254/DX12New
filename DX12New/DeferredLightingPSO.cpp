#include "DeferredLightingPSO.h"
#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

DeferredLightingPSO::DeferredLightingPSO(std::shared_ptr<Device> _device, bool _enableLighting)
	:BasePSO(_device),
	m_EnableLights(_enableLighting)
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\FullScreen_VS.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\HDR_PS.cso", &pixelShaderBlob));

	//设置根签名的标签,防止一些无必要的访问
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 3);

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::CameraPosCB].InitAsConstants(sizeof(Vector4) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::DirectionalLights].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init_1_1(RootParameters::NumRootParameters, rootParameter, 1, &anisotropicSampler, rootSignatureFlags);

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
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

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

DeferredLightingPSO::~DeferredLightingPSO()
{
}

void DeferredLightingPSO::Apply(CommandList& _commandList)
{
	//static bool isFirst = true;
	//if (m_pPreviousCommandList != &_commandList)
	//{
	//	_commandList.SetPipelineState(m_PSO);
	//	_commandList.SetGraphicsRootSignature(m_RootSignature);
	//	isFirst = false;
	//	m_pPreviousCommandList = &_commandList;
	//}
	_commandList.SetPipelineState(m_PSO);
	_commandList.SetGraphicsRootSignature(m_RootSignature);

	_commandList.SetGraphics32BitConstants(RootParameters::CameraPosCB, m_pAlignedMVP->CamPos.GetFloat4());
	//依次判断需要更新的属性,并绑定到渲染管线上
	if (m_DirtyFlags & DF_Material)
	{

		//设置贴图
		BindTexture(_commandList, 0, m_Textures[AlbedoText], RootParameters::Textures);
		BindTexture(_commandList, 1, m_Textures[NormalText], RootParameters::Textures);
		BindTexture(_commandList, 2, m_Textures[ORMText], RootParameters::Textures);
		BindTexture(_commandList, 3, m_Textures[EmissiveText], RootParameters::Textures);
		BindTexture(_commandList, 4, m_Textures[WorldPosText], RootParameters::Textures);

	}

	if (m_DirtyFlags & DF_PointLights)
	{
		_commandList.SetGraphicsDynamicStructuredBuffer(RootParameters::PointLights, m_PointLights);
	}

	if (m_DirtyFlags & DF_SpotLights)
	{
		_commandList.SetGraphicsDynamicStructuredBuffer(RootParameters::SpotLights, m_SpotLights);
	}

	if (m_DirtyFlags & DF_DirectionalLights)
	{
		_commandList.SetGraphicsDynamicStructuredBuffer(RootParameters::DirectionalLights, m_DirectionalLights);
	}

	if (m_DirtyFlags & (DF_PointLights | DF_SpotLights | DF_DirectionalLights))
	{
		LightProperties LightProps;
		LightProps.NumPointLights = static_cast<uint32_t>(m_PointLights.size());
		LightProps.NumSpotLights = static_cast<uint32_t>(m_SpotLights.size());
		LightProps.NumDirectionalLights = static_cast<uint32_t>(m_DirectionalLights.size());

		_commandList.SetGraphics32BitConstants(RootParameters::LightPropertiesCB, LightProps);
	}

	//清空标志
	m_DirtyFlags = DF_None;
}
