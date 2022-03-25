#include "Lesson3.h"
#include "Application.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "helpers.h"
#include "Light.h"
#include "Material.h"
#include "Window.h"
#include <iostream>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "d3dx12.h"
#include <d3dcompiler.h>
#include <DirectXColors.h>
using namespace DirectX;

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

enum TonemapMethod : uint32_t
{
	TM_Linear,
	TM_Reinhard,
	TM_ReinhardSq,
	TM_ACESFilmic,
};

struct TonemapParameters
{
	TonemapParameters()
		: TonemapMethod(TM_Reinhard)
		, Exposure(0.0f)
		, MaxLuminance(1.0f)
		, K(1.0f)
		, A(0.22f)
		, B(0.3f)
		, C(0.1f)
		, D(0.2f)
		, E(0.01f)
		, F(0.3f)
		, LinearWhite(11.2f)
		, Gamma(2.2f)
	{}

	// The method to use to perform tonemapping.
	TonemapMethod TonemapMethod;
	// Exposure should be expressed as a relative exposure value (-2, -1, 0, +1, +2 )
	float Exposure;

	// The maximum luminance to use for linear tonemapping.
	float MaxLuminance;

	// Reinhard constant. Generally this is 1.0.
	float K;

	// ACES Filmic parameters
	// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
	float A; // Shoulder strength
	float B; // Linear strength
	float C; // Linear angle
	float D; // Toe strength
	float E; // Toe Numerator
	float F; // Toe denominator
	// Note E/F = Toe angle.
	float LinearWhite;
	float Gamma;
};

TonemapParameters g_TonemapParameters;

struct Mat
{
	XMMATRIX ModelMatrix;
	XMMATRIX ModelViewMatrix;
	XMMATRIX InverseTransposeModelMatrix;
	XMMATRIX ModelViewProjMatrix;
};

struct LightProperties
{
	uint32_t NumPointLights;
	uint32_t NumSpotLights;
};

enum RootParameters
{
	MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
	MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
	LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );
	PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
	SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
	Textures,           // Texture2D DiffuseTexture : register( t2 );
	NumRootParameters
};

// Builds a look-at (world) matrix from a point, up and direction vectors.
XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR _position, FXMVECTOR _direction, FXMVECTOR  _up)
{
	assert(!XMVector3Equal(_direction, XMVectorZero()));
	assert(!XMVector3IsInfinite(_direction));
	assert(!XMVector3Equal(_up, XMVectorZero()));
	assert(!XMVector3IsInfinite(_up));

	XMVECTOR R2 = XMVector3Normalize(_direction);
	XMVECTOR R0 = XMVector3Cross(_up, R2);
	R0 = XMVector3Normalize(R0);

	XMVECTOR R1 = XMVector3Cross(R2, R0);

	XMMATRIX M(R0, R1, R2, _position);

	return M;
}

Lesson3::Lesson3(const std::wstring& _name, int _width, int _height, bool _vSync)
	:super(_name, _width, _height, _vSync),
	m_Rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0F, 0.0F, static_cast<float>(_width), static_cast<float>(_height))),
	m_Forward(0)
	, m_Backward(0)
	, m_Left(0)
	, m_Right(0)
	, m_Up(0)
	, m_Down(0)
	, m_Pitch(0)
	, m_Yaw(0)
	, m_AnimaeLights(false)
	, m_Shift(false)
	, m_Width(_width)
	, m_Height(_height)
{
	XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_Camera.SetLookAt(cameraPos, cameraTarget, cameraUp);
	float aspect = m_Width / (float)m_Height;
	m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);

	m_pAlignedCameraDate = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);

	m_pAlignedCameraDate->m_InitialCamPos = m_Camera.GetTranslation();
	m_pAlignedCameraDate->m_InitialCamRot = m_Camera.GetRotation();
	m_pAlignedCameraDate->m_InitialFov = m_Camera.GetFov();
}

Lesson3::~Lesson3()
{
	_aligned_free(m_pAlignedCameraDate);
}

bool Lesson3::LoadContent()
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	m_Cube = Mesh::CreateCube(*commandList);
	m_Sphere = Mesh::CreateSphere(*commandList);
	m_Cone = Mesh::CreateCone(*commandList);
	m_Torus = Mesh::CreateTorus(*commandList);
	m_Plane = Mesh::CreatePlane(*commandList);
	m_SkyBox = Mesh::CreateCube(*commandList);

	commandList->LoadTextureFromFile(m_DefaultTexture, L"C:/Code/DX12New/Textures/DefaultWhite.bmp");
	commandList->LoadTextureFromFile(m_DirectXTexture, L"C:/Code/DX12New/Textures/Directx9.png");
	commandList->LoadTextureFromFile(m_EarthTexture, L"C:/Code/DX12New/Textures/earth.dds");
	commandList->LoadTextureFromFile(m_MonalisaTexture, L"C:/Code/DX12New/Textures/Mona_Lisa.jpg");
	commandList->LoadTextureFromFile(m_GraceCathedralTexture, L"C:/Code/DX12New/Textures/grace-new.hdr");

	//从一个HDR全景图创建CUBEMAP
	auto cubemapDesc = m_GraceCathedralTexture.GetResourceDesc();
	cubemapDesc.Width = cubemapDesc.Height = 1024;
	cubemapDesc.DepthOrArraySize = 6;
	cubemapDesc.MipLevels = 0;

	m_GraceCathedralCubemap = Texture(cubemapDesc, nullptr, TextureUsage::Albedo, L"Cubemap");
	//转换
	commandList->PanoToCubeMap(m_GraceCathedralCubemap, m_GraceCathedralTexture);

	//创建一个HDR中间层的渲染目标
	DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//创建一个离线的渲染目标，有一个颜色和一个深度
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat, m_Width, m_Height);
	colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clear;
	clear.Format = colorDesc.Format;
	clear.Color[0] = 0.4f;
	clear.Color[1] = 0.6f;
	clear.Color[2] = 0.9f;
	clear.Color[3] = 1.0f;

	Texture HDRTexture = Texture(colorDesc, &clear, TextureUsage::RenderTarget, L"HDR Texture");

	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		depthBufferFormat,
		m_Width, m_Height,
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE depthClear;
	depthClear.Format = depthDesc.Format;
	depthClear.DepthStencil = { 1.0f, 0 };

	Texture DepthTexture = Texture(depthDesc, &depthClear, TextureUsage::Depth, L"DepthRT");

	m_HDRRenderTarget.AttachTexture(AttachmentPoint::Color0, HDRTexture);
	m_HDRRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, DepthTexture);

	//创建根签名
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//允许输入布局，并拒绝某些管线不必要的访问
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	//创建一个根签名和PSO用于skybox
	{
		ComPtr<ID3DBlob> skyBoxVS;
		ComPtr<ID3DBlob> skyBoxPS;
		ThrowIfFailed(D3DReadFileToBlob(L"SkyBox_VS.cso", &skyBoxVS));
		ThrowIfFailed(D3DReadFileToBlob(L"SkyBox_PS.cso", &skyBoxPS));

		//输入描述
		D3D12_INPUT_ELEMENT_DESC inputLayout[1] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, };

		//设置根参数
		//描述符表
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRage(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		//创建根参数
		CD3DX12_ROOT_PARAMETER1 skyRootParameter[2];
		skyRootParameter[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		skyRootParameter[1].InitAsDescriptorTable(1, &descriptorRage, D3D12_SHADER_VISIBILITY_PIXEL);

		//线性钳制采样器
		CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(0,
			D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		//设置根签名
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC skyRootSignatureDesc;
		skyRootSignatureDesc.Init_1_1(2, skyRootParameter, 1, &linearClampSampler, rootSignatureFlags);

		m_SkyboxRootSignature.SetRootSignatureDesc(skyRootSignatureDesc.Desc_1_1, featureData.HighestVersion);

		//天空盒PSO
		struct SkyboxPipelineState
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterzer;
		} skyboxPipelineStateStream;

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

		skyboxPipelineStateStream.pRootSignature = m_SkyboxRootSignature.GetRootSignature().Get();
		skyboxPipelineStateStream.InputLayout = { inputLayout, 1 };
		skyboxPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		skyboxPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(skyBoxVS.Get());
		skyboxPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(skyBoxPS.Get());
		skyboxPipelineStateStream.RTVFormats = m_HDRRenderTarget.GetRenderTargetFormats();
		skyboxPipelineStateStream.Rasterzer = rasterizerDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC skyboxPSODesc = { sizeof(skyboxPipelineStateStream), &skyboxPipelineStateStream };

		ThrowIfFailed(device->CreatePipelineState(&skyboxPSODesc, IID_PPV_ARGS(&m_SkyPSO)));
	}

	//创建一个根签名用于HDR渲染
	{
		ComPtr<ID3DBlob> hdrVS;
		ComPtr<ID3DBlob> hdrPS;
		ThrowIfFailed(D3DReadFileToBlob(L"StandardVS.cso", &hdrVS));
		ThrowIfFailed(D3DReadFileToBlob(L"HDR_PS.cso", &hdrPS));

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

		//根参数
		CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
		rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[RootParameters::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[RootParameters::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		//静态采样器
		CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		//设置根签名
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);
		m_HDRRootSignature.SetRootSignatureDesc(rootSignatureDesc.Desc_1_1, featureData.HighestVersion);

		//PSO
		struct PipeLineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SamplerDesc;
		}pipeLineStateStream;

		//pso描述结构体
		pipeLineStateStream.pRootSignature = m_HDRRootSignature.GetRootSignature().Get();
		pipeLineStateStream.InputLayout = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
		pipeLineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipeLineStateStream.VS = CD3DX12_SHADER_BYTECODE(hdrVS.Get());
		pipeLineStateStream.PS = CD3DX12_SHADER_BYTECODE(hdrPS.Get());
		pipeLineStateStream.DSVFormat = m_HDRRenderTarget.GetDepthStencilFormat();
		pipeLineStateStream.RTVFormat = m_HDRRenderTarget.GetRenderTargetFormats();
		pipeLineStateStream.SamplerDesc = { 1, 0 };

		D3D12_PIPELINE_STATE_STREAM_DESC psoDesc = { sizeof(PipeLineStateStream), &pipeLineStateStream };

		//创建PSO
		ThrowIfFailed(device->CreatePipelineState(&psoDesc, IID_PPV_ARGS(&m_HDRPSO)));
	}

	//创建SDR使用的根签名和PSO
	{
	//加载着色器
	ComPtr<ID3DBlob> vsBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"SDR_VS.cso", &vsBlob));
	ComPtr<ID3DBlob> psBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"SDR_PS.cso", &psBlob));

	//描述符表
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRage(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(TonemapParameters) / 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &descriptorRage, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearClampsSampler(0, 
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init_1_1(2, rootParameters, 1, &linearClampsSampler);

	m_SDRRootSignature.SetRootSignatureDesc(rsDesc.Desc_1_1, featureData.HighestVersion);

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	struct SDRPipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} sdrPipelineStateStream;

	sdrPipelineStateStream.pRootSignature = m_SDRRootSignature.GetRootSignature().Get();
	sdrPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	sdrPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	sdrPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	sdrPipelineStateStream.Rasterizer = rasterizerDesc;
	sdrPipelineStateStream.RTVFormats = m_pWindow->GetRenderTarget().GetRenderTargetFormats();
	
	D3D12_PIPELINE_STATE_STREAM_DESC sdrPipelineStateStreamDesc = { sizeof(SDRPipelineStateStream), &sdrPipelineStateStream };

	ThrowIfFailed(device->CreatePipelineState(&sdrPipelineStateStreamDesc, IID_PPV_ARGS(&m_SDRPSO)));
	}

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	return true;
}

void Lesson3::UnLoadContent()
{

}

void Lesson3::OnUpdate(UpdateEventArgs& _e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(_e);

	totalTime += _e.ElapsedTime;
	frameCount++;

	//计算帧数
	if (totalTime > 1.0f)
	{
		double fps = frameCount / totalTime;

		//system("cls");
		std::cout << "FPS: " << fps << std::endl;

		frameCount = 0;
		totalTime = 0.0f;
	}

	float speedMultipler = (m_Shift ? 16.0f : 4.0f);

	XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(_e.ElapsedTime);
	XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(_e.ElapsedTime);
	m_Camera.Translate(cameraTranslate, Space::Local);
	m_Camera.Translate(cameraPan, Space::Local);

	XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
	m_Camera.SetRotation(cameraRotation);

	XMMATRIX viewMatrix = m_Camera.GetViewMatrix();

	const int numPointLights = 4;
	const int numSpotLights = 4;

	static const XMVECTORF32 LightColors[] =
	{
		Colors::White, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Indigo, Colors::Violet, Colors::White
	};

	static float lightAnimTime = 0.0f;
	if (m_AnimaeLights)
	{
		lightAnimTime += static_cast<float>(_e.ElapsedTime) * 0.5f * XM_PI;
	}

	const float radius = 8.0f;
	const float offset = 2.0f * XM_PI / numPointLights;
	const float offset2 = offset + (offset / 2.0f);

	// Setup the light buffers.
	m_PointLights.resize(numPointLights);
	for (int i = 0; i < numPointLights; ++i)
	{
		PointLight& l = m_PointLights[i];

		l.PositionWS = {
			static_cast<float>(std::sin(lightAnimTime + offset * i)) * radius,
			9.0f,
			static_cast<float>(std::cos(lightAnimTime + offset * i)) * radius,
			1.0f
		};
		XMVECTOR positionWS = XMLoadFloat4(&l.PositionWS);
		XMVECTOR positionVS = XMVector3TransformCoord(positionWS, viewMatrix);
		XMStoreFloat4(&l.PositionVS, positionVS);

		l.Color = XMFLOAT4(LightColors[i]);
		l.ConstantAttenuation = 1.0f;
		l.LinearAttenuation = 0.08f;
		l.QuadraticAttenuation = 0.0f;
	}

	m_SpotLights.resize(numSpotLights);
	for (int i = 0; i < numSpotLights; ++i)
	{
		SpotLight& l = m_SpotLights[i];

		l.PositionWS = {
			static_cast<float>(std::sin(lightAnimTime + offset * i + offset2)) * radius,
			9.0f,
			static_cast<float>(std::cos(lightAnimTime + offset * i + offset2)) * radius,
			1.0f
		};
		XMVECTOR positionWS = XMLoadFloat4(&l.PositionWS);
		XMVECTOR positionVS = XMVector3TransformCoord(positionWS, viewMatrix);
		XMStoreFloat4(&l.PositionVS, positionVS);

		XMVECTOR directionWS = XMVector3Normalize(XMVectorSetW(XMVectorNegate(positionWS), 0));
		XMVECTOR directionVS = XMVector3Normalize(XMVector3TransformNormal(directionWS, viewMatrix));
		XMStoreFloat4(&l.DirectionWS, directionWS);
		XMStoreFloat4(&l.DirectionVS, directionVS);

		l.Color = XMFLOAT4(LightColors[numPointLights + i]);
		l.SpotAngle = XMConvertToRadians(45.0f);
		l.ConstantAttenuation = 1.0f;
		l.LinearAttenuation = 0.08f;
		l.QuadraticAttenuation = 0.0f;
	}
}

void XM_CALLCONV ComputerMatrices(FXMMATRIX _model, CXMMATRIX _view, CXMMATRIX _viewProj, Mat& _mat)
{
	_mat.ModelMatrix = _model;
	_mat.ModelViewMatrix = _model * _view;
	_mat.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, _mat.ModelViewMatrix));
	_mat.ModelViewProjMatrix = _model * _viewProj;
}

void Lesson3::OnRender(RenderEventArgs& _e)
{

	super::OnRender(_e);

		//获取所需的对象
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	//清除缓冲区
	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	commandList->ClearTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
	commandList->ClearDepthStencilTexture(m_HDRRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

	//设置视口
	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_Rect);
	//设置渲染目标
	commandList->SetRenderTarget(m_HDRRenderTarget);
	
	//渲染天空盒
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
		XMMATRIX rotationMatrix = XMMatrixIdentity();
		XMMATRIX scaleMatrix = XMMatrixScaling(400.0f, 400.0f, 400.0f);
		XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		auto viewMat = m_Camera.GetViewMatrix();//XMMatrixTranspose(XMMatrixRotationQuaternion(m_Camera.GetRotation()));
		auto projMat = m_Camera.GetProjMatrix();
		auto viewProjMat = worldMatrix * viewMat * projMat;

		commandList->SetPipelineState(m_SkyPSO);
		commandList->SetGraphicsRootSignature(m_SkyboxRootSignature);

		commandList->SetGraphics32BitConstants(0, viewProjMat);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_GraceCathedralCubemap.GetResourceDesc().Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = (UINT)-1;//使用全部Mips

		commandList->SetShaderResourceView(1, 0, m_GraceCathedralCubemap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &srvDesc);

		m_SkyBox->Draw(*commandList);
	}
	//设置根签名和PSO
	commandList->SetPipelineState(m_HDRPSO);
	commandList->SetGraphicsRootSignature(m_HDRRootSignature);

	//更新灯光
	LightProperties lightProps;
	lightProps.NumPointLights = static_cast<uint32_t>(m_PointLights.size());
	lightProps.NumSpotLights = static_cast<uint32_t>(m_SpotLights.size());

	//设置灯光数据
	commandList->SetGraphics32BitConstants(RootParameters::LightPropertiesCB, lightProps);
	commandList->SetGraphicsDynamicStructuredBuffer(RootParameters::PointLights, m_PointLights);
	commandList->SetGraphicsDynamicStructuredBuffer(RootParameters::SpotLights, m_SpotLights);

	//渲染球体
	XMMATRIX translationMatrix = XMMatrixTranslation(-4.0f, 2.0f, -4.0f);
	XMMATRIX rotationMatrix = XMMatrixIdentity();
	XMMATRIX scaleMatrix = XMMatrixScaling(4.0f, 4.0f, 4.0f);
	XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	XMMATRIX viewMatrix = m_Camera.GetViewMatrix();
	XMMATRIX viewProjectionMatrix = viewMatrix * m_Camera.GetProjMatrix();

	Mat matrices;
	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
	commandList->SetShaderResourceView(RootParameters::Textures, 0, m_EarthTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_Sphere->Draw(*commandList);

	// Draw a cube
	translationMatrix = XMMatrixTranslation(4.0f, 4.0f, 4.0f);
	rotationMatrix = XMMatrixRotationY(XMConvertToRadians(45.0f));
	scaleMatrix = XMMatrixScaling(4.0f, 8.0f, 4.0f);
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
	commandList->SetShaderResourceView(RootParameters::Textures, 0, m_MonalisaTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_Cube->Draw(*commandList);

	// Draw a torus
	translationMatrix = XMMatrixTranslation(4.0f, 0.6f, -4.0f);
	rotationMatrix = XMMatrixRotationY(XMConvertToRadians(45.0f));
	scaleMatrix = XMMatrixScaling(4.0f, 4.0f, 4.0f);
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Ruby);
	commandList->SetShaderResourceView(RootParameters::Textures, 0, m_DefaultTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_Torus->Draw(*commandList);

	// Floor plane.
	float scalePlane = 20.0f;
	float translateOffset = scalePlane / 2.0f;

	translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	rotationMatrix = XMMatrixIdentity();
	scaleMatrix = XMMatrixScaling(scalePlane, 1.0f, scalePlane);
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
	commandList->SetShaderResourceView(RootParameters::Textures, 0, m_DirectXTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_Plane->Draw(*commandList);

	// Back wall
	translationMatrix = XMMatrixTranslation(0, translateOffset, translateOffset);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);

	m_Plane ->Draw(*commandList);

	// Ceiling plane
	translationMatrix = XMMatrixTranslation(0, translateOffset * 2.0f, 0);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(180));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);

	m_Plane->Draw(*commandList);

	// Front wall
	translationMatrix = XMMatrixTranslation(0, translateOffset, -translateOffset);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(90));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);

	m_Plane->Draw(*commandList);

	// Left wall
	translationMatrix = XMMatrixTranslation(-translateOffset, translateOffset, 0);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Red);
	commandList->SetShaderResourceView(RootParameters::Textures, 0, m_DefaultTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_Plane->Draw(*commandList);

	// Right wall
	translationMatrix = XMMatrixTranslation(translateOffset, translateOffset, 0);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Blue);
	m_Plane->Draw(*commandList);

	// Draw shapes to visualize the position of the lights in the scene.
	Material lightMaterial;
	// No specular
	lightMaterial.Specular = { 0, 0, 0, 1 };
	for (const auto& l : m_PointLights)
	{
		lightMaterial.Emissive = l.Color;
		XMVECTOR lightPos = XMLoadFloat4(&l.PositionWS);
		worldMatrix = XMMatrixTranslationFromVector(lightPos);
		ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, lightMaterial);

		m_Sphere->Draw(*commandList);
	}

	for (const auto& l : m_SpotLights)
	{
		lightMaterial.Emissive = l.Color;
		XMVECTOR lightPos = XMLoadFloat4(&l.PositionWS);
		XMVECTOR lightDir = XMLoadFloat4(&l.DirectionWS);
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		// Rotate the cone so it is facing the Z axis.
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90.0f));
		worldMatrix = rotationMatrix * LookAtMatrix(lightPos, lightDir, up);

		ComputerMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, lightMaterial);

		m_Cone->Draw(*commandList);
	}

	commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
	commandList->SetViewport(m_pWindow->GetRenderTarget().GetViewport());
	commandList->SetPipelineState(m_SDRPSO);
	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootSignature(m_SDRRootSignature);
	commandList->SetGraphics32BitConstants(0, g_TonemapParameters);
	commandList->SetShaderResourceView(1, 0, m_HDRRenderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->Draw(3);

	commandQueue->ExecuteCommandList(commandList);

	m_pWindow->Present();
}

static bool g_AllowFullscreenToggle = true;

void Lesson3::OnKeyPressed(KeyEventArgs& _e)
{
	super::OnKeyPressed(_e);

	switch (_e.key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;
	case KeyCode::Enter:
		if (_e.alt)
		{
	case KeyCode::F11:
		m_pWindow->ToggleFullScreen();
		break;
		}
	case KeyCode::V:
		m_pWindow->ToggleVSync();
		break;
	case KeyCode::R:
		// Reset camera transform
		m_Camera.SetTranslation(m_pAlignedCameraDate->m_InitialCamPos);
		m_Camera.SetRotation(m_pAlignedCameraDate->m_InitialCamRot);
		m_Camera.SetFov(m_pAlignedCameraDate->m_InitialFov);
		m_Pitch = 0.0f;
		m_Yaw = 0.0f;
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 1.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 1.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 1.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 1.0f;
		break;
	case KeyCode::Q:
		m_Down = 1.0f;
		break;
	case KeyCode::E:
		m_Up = 1.0f;
		break;
	case KeyCode::Space:
		m_AnimaeLights = !m_AnimaeLights;
		break;
	case KeyCode::ShiftKey:
		m_Shift = true;
		break;
	}
}

void Lesson3::OnKeyReleased(KeyEventArgs& _e)
{
	super::OnKeyReleased(_e);

	switch (_e.key)
	{
	case KeyCode::Enter:
		if (_e.alt)
		{
	case KeyCode::F11:
		g_AllowFullscreenToggle = true;
		}
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 0.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 0.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 0.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 0.0f;
		break;
	case KeyCode::Q:
		m_Down = 0.0f;
		break;
	case KeyCode::E:
		m_Up = 0.0f;
		break;
	case KeyCode::ShiftKey:
		m_Shift = false;
		break;
	}
}


void Lesson3::OnMouseMoved(MouseMotionEventArgs& _e)
{
	super::OnMouseMoved(_e);

	const float mouseSpeed = 0.1f;
	if (_e.LeftButton)
	{
		m_Pitch += _e.RelY * mouseSpeed;

		m_Pitch = Clamp(m_Pitch, -90.0f, 90.0f);

		m_Yaw += _e.RelX * mouseSpeed;
	}
}

void Lesson3::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{

}

void Lesson3::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{

}

void Lesson3::OnMouseWheel(MouseWheelEventArgs& _e)
{
}

void Lesson3::OnResize(ResizeEventArgs& _e)
{
	super::OnResize(_e);

	if (_e.Width != GetWidth() || _e.Height != GetHeight())
	{
		m_Width = std::max(1, _e.Width);
		m_Height = std::max(1, _e.Height);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

		float aspect = m_Width / (float)m_Height;
		m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);

		m_SDRRenderTarget.Resize(m_Width, m_Height);
	}
}

void Lesson3::OnWindowDestroy()
{

}
