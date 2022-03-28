#include "EffectPSO.h"


#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

EffectPSO::EffectPSO(std::shared_ptr<Device> _device, bool _enableLighting, bool _enableDecal)
	:m_Device(_device),
	m_DirtyFlags(DF_All),
	m_pPreviousCommandList(nullptr),
	m_EnableLights(_enableLighting),
	m_EnableDecal(_enableDecal)
{
	m_pAlignedMVP = (MVP*)_aligned_malloc(sizeof(MVP), 16);

	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\StandardVS.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	if (m_EnableLights)
	{
		if (m_EnableDecal)
		{
			ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\Decal.cso", &pixelShaderBlob));
		}
		else
		{
			ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\HDR_PS.cso", &pixelShaderBlob));
		}
	}
	else
	{
		ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\Unlit.cso", &pixelShaderBlob));
	}

	//���ø�ǩ���ı�ǩ,��ֹһЩ�ޱ�Ҫ�ķ���
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 3);

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameter[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
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
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
	} pipelineStateStream;

	//����һ������SRGB����ɫ����,Ϊ��gamma����
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//��ȡ���ز�����֧��
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
	if (m_EnableDecal) {
		//��Ⱦmask����ʱ,�رձ����޳�
		rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	}

	//����PSO����
	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.RasterizerState = rasterizerState;
	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = sampleDesc;

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);

	//����Ĭ�ϵĿհ�SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC defaultSRV;
	defaultSRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	defaultSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	defaultSRV.Texture2D.MostDetailedMip = 0;
	defaultSRV.Texture2D.MipLevels = 1;
	defaultSRV.Texture2D.PlaneSlice = 0;
	defaultSRV.Texture2D.ResourceMinLODClamp = 0;
	defaultSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_DefaultSRV = m_Device->CreateShaderResourceView(nullptr, &defaultSRV);
}

EffectPSO::~EffectPSO()
{
	_aligned_free(m_pAlignedMVP);
}

void EffectPSO::Apply(CommandList& _commandList)
{
	_commandList.SetPipelineState(m_PSO);
	_commandList.SetGraphicsRootSignature(m_RootSignature);

	//�����ж���Ҫ���µ�����,���󶨵���Ⱦ������
	if (m_DirtyFlags & DF_Matrices)
	{
		Matrices m;
		m.ModelMatrix = m_pAlignedMVP->World;
		m.ModelViewMatrix = m_pAlignedMVP->World * m_pAlignedMVP->View;
		m.ModelViewProjectionMatrix = m.ModelViewMatrix * m_pAlignedMVP->Projection;
		m.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m.ModelMatrix));

		_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, m);
	}

	if (m_DirtyFlags & DF_Material)
	{
		if (m_Material)
		{
			const auto materialProps = m_Material->GetMaterialProperties();

			_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, materialProps);

			//������ͼ
			BindTexture(_commandList, 0, m_Material->GetTexture(Material::TextureType::Ambient));
			BindTexture(_commandList, 1, m_Material->GetTexture(Material::TextureType::Emissive));
			BindTexture(_commandList, 2, m_Material->GetTexture(Material::TextureType::Diffuse));
			BindTexture(_commandList, 3, m_Material->GetTexture(Material::TextureType::Specular));
			BindTexture(_commandList, 4, m_Material->GetTexture(Material::TextureType::SpecularPower));
			BindTexture(_commandList, 5, m_Material->GetTexture(Material::TextureType::Normal));
			BindTexture(_commandList, 6, m_Material->GetTexture(Material::TextureType::Bump));
			BindTexture(_commandList, 7, m_Material->GetTexture(Material::TextureType::Opacity));
		}
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

	//��ձ�־
	m_DirtyFlags = DF_None;
}

void EffectPSO::BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture)
{
	//�����ͼ��Ч��������ͼ,�������Ĭ�ϵ�SRV���
	if (_texture)
	{
		_commandList.SetShaderResourceView(RootParameters::Textures, _offset, _texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		_commandList.SetShaderResourceView(RootParameters::Textures, _offset, m_DefaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}
