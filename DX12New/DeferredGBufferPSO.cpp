#include "DeferredGBufferPSO.h"

#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

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


	//���ø�ǩ���ı�ǩ,��ֹһЩ�ޱ�Ҫ�ķ���
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
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
	} pipelineStateStream;

	//����һ������SRGB����ɫ����,Ϊ��gamma����
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//��ȡ���ز�����֧��
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 5;
	rtvFormats.RTFormats[0] = backBufferFormat;
	rtvFormats.RTFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvFormats.RTFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvFormats.RTFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvFormats.RTFormats[4] = DXGI_FORMAT_R16G16B16A16_FLOAT;

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
	pipelineStateStream.SampleDesc = {1, 0};

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

void XM_CALLCONV DeferredGBufferPSO::SetWorldMatrix(Matrix4 worldMatrix)
{
	m_pAlignedMVP->World = worldMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 DeferredGBufferPSO::GetWorldMatrix() const
{
	return m_pAlignedMVP->World;
}

void XM_CALLCONV DeferredGBufferPSO::SetViewMatrix(Matrix4 viewMatrix)
{
	m_pAlignedMVP->View = viewMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 DeferredGBufferPSO::GetViewMatrix() const
{
	return m_pAlignedMVP->View;
}

void XM_CALLCONV DeferredGBufferPSO::SetProjectionMatrix(Matrix4 projectionMatrix)
{
	m_pAlignedMVP->Projection = projectionMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 DeferredGBufferPSO::GetProjectionMatrix() const
{
	return m_pAlignedMVP->Projection;
}

void DeferredGBufferPSO::Apply(CommandList& _commandList)
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
		m.InverseTransposeModelViewMatrix = Transform::MatrixTranspose(Transform::InverseMatrix(nullptr, m.ModelMatrix));

		_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, m);
	}

	if (m_DirtyFlags & DF_Material)
	{
		if (m_Material)
		{
			const auto materialProps = m_Material->GetMaterialProperties();

			_commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, materialProps);

			//������ͼ
			BindTexture(_commandList, 0, m_Material->GetTexture(Material::TextureType::AO), RootParameters::Textures);
			BindTexture(_commandList, 1, m_Material->GetTexture(Material::TextureType::Emissive), RootParameters::Textures);
			BindTexture(_commandList, 2, m_Material->GetTexture(Material::TextureType::Diffuse), RootParameters::Textures);
			BindTexture(_commandList, 3, m_Material->GetTexture(Material::TextureType::Metaltic), RootParameters::Textures);
			BindTexture(_commandList, 4, m_Material->GetTexture(Material::TextureType::Roughness), RootParameters::Textures);
			BindTexture(_commandList, 5, m_Material->GetTexture(Material::TextureType::Normal), RootParameters::Textures);
			BindTexture(_commandList, 6, m_Material->GetTexture(Material::TextureType::Bump), RootParameters::Textures);
			BindTexture(_commandList, 7, m_Material->GetTexture(Material::TextureType::Opacity), RootParameters::Textures);
		}
	}

	//��ձ�־
	m_DirtyFlags = DF_None;
}

