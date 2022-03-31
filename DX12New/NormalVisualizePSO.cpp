#include "NormalVisualizePSO.h"
#include "d3dx12.h"
#include <d3dcompiler.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

NormalVisualizePSO::NormalVisualizePSO(std::shared_ptr<Device> _device)
	:BasePSO(_device)
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\NormalVisualize_VS.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> geometryShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\NormalVisualize_GS.cso", &geometryShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"C:\\Code\\DX12New\\x64\\Debug\\Unlit_PS.cso", &pixelShaderBlob));


	//���ø�ǩ���ı�ǩ,��ֹһЩ�ޱ�Ҫ�ķ���
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameter[RootParameters::NumRootParameters];
	rootParameter[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init_1_1(RootParameters::NumRootParameters, rootParameter, 0, nullptr, rootSignatureFlags);

	m_RootSignature = m_Device->CreateRootSignature(rsDesc.Desc_1_1);

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
		CD3DX12_PIPELINE_STATE_STREAM_GS                    GS;
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


	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = backBufferFormat;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);

	//����PSO����
	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.GS = CD3DX12_SHADER_BYTECODE(geometryShaderBlob.Get());
	pipelineStateStream.RasterizerState = rasterizerState;
	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.SampleDesc = { 1, 0 };

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);
}

NormalVisualizePSO::~NormalVisualizePSO()
{
}

void XM_CALLCONV NormalVisualizePSO::SetWorldMatrix(Matrix4 worldMatrix)
{
	m_pAlignedMVP->World = worldMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 NormalVisualizePSO::GetWorldMatrix() const
{
	return m_pAlignedMVP->World;
}

void XM_CALLCONV NormalVisualizePSO::SetViewMatrix(Matrix4 viewMatrix)
{
	m_pAlignedMVP->View = viewMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 NormalVisualizePSO::GetViewMatrix() const
{
	return m_pAlignedMVP->View;
}

void XM_CALLCONV NormalVisualizePSO::SetProjectionMatrix(Matrix4 projectionMatrix)
{
	m_pAlignedMVP->Projection = projectionMatrix;
	m_DirtyFlags |= DF_Matrices;
}

Matrix4 NormalVisualizePSO::GetProjectionMatrix() const
{
	return m_pAlignedMVP->Projection;
}

void NormalVisualizePSO::Apply(CommandList& _commandList)
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

	//��ձ�־
	m_DirtyFlags = DF_None;
}
