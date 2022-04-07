#include "DeferredGBufferPSO.h"

#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>
#include <iostream>

using namespace Microsoft::WRL;
using namespace DirectX;

DeferredGBufferPSO::DeferredGBufferPSO(std::shared_ptr<Device> _device, bool _enableDecal)
	:BasePSO(_device),
	m_EnableDecal(_enableDecal)
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\StandardVS.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;

	if (m_EnableDecal)
	{
		ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\Decal.cso", &pixelShaderBlob));
	}
	else
	{
		ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\GBuffer_PS.cso", &pixelShaderBlob));
	}


	//设置根签名的标签,防止一些无必要的访问
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::ObjectCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameter[RootParameters::MainPassCB].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameter[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
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
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1        DepthStencil;
	} pipelineStateStream;

	//创建一个具有SRGB的颜色缓冲,为了gamma矫正
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//获取多重采样的支持
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 6;
	rtvFormats.RTFormats[0] = backBufferFormat;
	rtvFormats.RTFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvFormats.RTFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvFormats.RTFormats[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvFormats.RTFormats[4] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvFormats.RTFormats[5] = DXGI_FORMAT_R16G16_FLOAT;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
	if (m_EnableDecal) {
		//渲染mask物体时,关闭背面剔除
		rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	}

	//设置PSO属性
	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.RasterizerState = rasterizerState;
	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = {1, 0};

	CD3DX12_DEPTH_STENCIL_DESC1 p = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
	p.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
#if USE_REVERSE_Z
	p.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
#else
	p.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
#endif
	pipelineStateStream.DepthStencil = p;

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);
}

DeferredGBufferPSO::~DeferredGBufferPSO()
{
}

const std::shared_ptr<Material>& DeferredGBufferPSO::GetMaterial() const
{
	return m_Material;
}

void DeferredGBufferPSO::SetMaterial(const std::shared_ptr<Material>& _material)
{
	m_Material = _material;
	m_DirtyFlags |= DF_Material;
}

void DeferredGBufferPSO::Apply(CommandList& _commandList)
{
	_commandList.SetPipelineState(m_PSO);
	_commandList.SetGraphicsRootSignature(m_RootSignature);

	//依次判断需要更新的属性,并绑定到渲染管线上
	if (m_DirtyFlags & DF_ObjectCB)
	{
		_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::ObjectCB, *m_pAlignedObjectCB);
	}

	if (m_DirtyFlags & DF_MainPassCB)
	{
		_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MainPassCB, *m_pAlignedMainPassCB);
	}

	if (m_DirtyFlags & DF_Material || m_Material->IsDirty())
	{
		if (m_Material)
		{
			const auto materialProps = m_Material->GetMaterialProperties();

			_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, materialProps);
			//std::cout << m_Material->IsDirty() << std::endl;
			//设置贴图
			BindTexture(_commandList, 0, m_Material->GetTexture(Material::TextureType::ORM), RootParameters::Textures);
			BindTexture(_commandList, 1, m_Material->GetTexture(Material::TextureType::Emissive), RootParameters::Textures);
			BindTexture(_commandList, 2, m_Material->GetTexture(Material::TextureType::Diffuse), RootParameters::Textures);
			BindTexture(_commandList, 3, m_Material->GetTexture(Material::TextureType::Normal), RootParameters::Textures);
			BindTexture(_commandList, 4, m_Material->GetTexture(Material::TextureType::Bump), RootParameters::Textures);
			BindTexture(_commandList, 5, m_Material->GetTexture(Material::TextureType::Opacity), RootParameters::Textures);

			m_Material->SetDirty(false);
		}
	}

	//清空标志
	m_DirtyFlags = DF_None;
}

