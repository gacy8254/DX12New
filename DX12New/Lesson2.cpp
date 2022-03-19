#include "Lesson2.h"

#include "Application.h"
#include "CommandQueue.h"
#include "helpers.h"
#include "Window.h"

#include <wrl.h>
using namespace Microsoft::WRL;

#include "d3dx12.h"
#include <d3dcompiler.h>

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using namespace DirectX;

template<typename T>
constexpr const T& Clamp(const T& _val, const T& _min, const T& _max)
{
	return _val < _min ? _min : _val > _max ? _max : _val;
}

struct VertexPosColor 
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};


static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

Lesson2::Lesson2(const std::wstring& _name, int _width, int _height, bool _vSync)
	:Game(_name, _width, _height, _vSync),
	m_Rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0F, 0.0F, static_cast<float>(_width), static_cast<float>(_height))),
	m_FOV(45.0f),
	m_ContentLoaded(false)
{
}

bool Lesson2::LoadContent()
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandlist = commandQueue->GetCommandList();

	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(commandlist.Get(), &m_VertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);
	//创建vbv
	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandlist.Get(), &m_IndexBuffer, &intermediateIndexBuffer, _countof(g_Indicies), sizeof(WORD), g_Indicies);
	//创建IBV
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);

	//创建一个深度缓冲
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeap = {};
	dsvHeap.NumDescriptors = 1;
	dsvHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeap, IID_PPV_ARGS(&m_DSVHeap)));

	//加载着色器
	ComPtr<ID3DBlob> vertexBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexBlob));

	ComPtr<ID3DBlob> pixelBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelBlob));

	//着色器布局
	D3D12_INPUT_ELEMENT_DESC inputLayer[] =
	{
		{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"Color", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	//根签名
	{
		//检查支持的根签名版本
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		//根签名的着色器阶段可见性标志
		D3D12_ROOT_SIGNATURE_FLAGS rooTsignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		//根参数
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		//根签名描述
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rooTsignatureFlags);

		//根签名序列化
		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&desc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		//创建根签名
		ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
	}
	
	//创建PSO
	{
		struct PipeLineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormat;
		}pipeLineStateStream;

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		pipeLineStateStream.pRootSignature = m_RootSignature.Get();
		pipeLineStateStream.InputLayout = { inputLayer, _countof(inputLayer) };
		pipeLineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipeLineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexBlob.Get());
		pipeLineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelBlob.Get());
		pipeLineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipeLineStateStream.RTVFormat = rtvFormats;

		D3D12_PIPELINE_STATE_STREAM_DESC psoDesc = { sizeof(pipeLineStateStream), &pipeLineStateStream };
		ThrowIfFailed(device->CreatePipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
	}

	//执行命令，并等待命令完成
	auto fenceValue = commandQueue->ExecuteCommandList(commandlist);
	commandQueue->WaitForFenceValue(fenceValue);

	m_ContentLoaded = true;

	//重置深度缓冲的大小
	ResizeDepthBuffer(GetWidth(), GetHeight());


	return true;
}

void Lesson2::UnLoadContent()
{

}

void Lesson2::OnUpdate(UpdateEventArgs& _e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(_e);

	totalTime += _e.ElapsedTime;
	frameCount++;

	if (totalTime > 1.0f)
	{
		double fps = frameCount / totalTime;

		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		WCHAR wszClassName[256];
		memset(wszClassName, 0, sizeof(wszClassName));
		MultiByteToWideChar(CP_ACP, 0, buffer, static_cast<int>(strlen(buffer) + 1), wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));
		OutputDebugString(wszClassName);

		frameCount = 0;
		totalTime = 0.0f;
	}

	//计算矩阵
	float angle = static_cast<float>(_e.TotalTime * 90.0f);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMat = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = GetWidth() / static_cast<float>(GetHeight());
	m_ProjMat = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), aspectRatio, 0.1f, 1000.0f);
}

void Lesson2::OnRender(RenderEventArgs& _e)
{
	super::OnRender(_e);

	//获取所需的对象
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	UINT currentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	auto backBuffer = m_pWindow->GetCurrentBackBuffer();
	auto rtv = m_pWindow->GetCurrentRTV();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	//转换资源状态
	TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//清除缓冲区
	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	ClearRTV(commandList, rtv, clearColor);
	ClearDepth(commandList, dsv);

	commandList->SetPipelineState(m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_Rect);

	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

	XMMATRIX mvpMat = XMMatrixMultiply(m_ModelMat, m_ViewMat);
	mvpMat = XMMatrixMultiply(mvpMat, m_ProjMat);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMat, 0);

	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

	currentBackBufferIndex = m_pWindow->Present();

	commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);

}

void Lesson2::OnKeyPressed(KeyEventArgs& _e)
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
	}
}

void Lesson2::OnKeyReleased(KeyEventArgs& _e)
{

}

void Lesson2::OnMouseMoved(MouseMotionEventArgs& _e)
{

}

void Lesson2::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{

}

void Lesson2::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{

}

void Lesson2::OnMouseWheel(MouseWheelEventArgs& _e)
{
	m_FOV -= _e.WheelDelta;
	m_FOV = Clamp(m_FOV, 12.0f, 90.0f);
}

void Lesson2::OnResize(ResizeEventArgs& _e)
{
	if (_e.Width != GetWidth() || _e.Height != GetHeight())
	{
		super::OnResize(_e);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(_e.Width), static_cast<float>(_e.Height));

		ResizeDepthBuffer(_e.Width, _e.Height);
	}
}

void Lesson2::OnWindowDestroy()
{

}

void Lesson2::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList, 
	Microsoft::WRL::ComPtr<ID3D12Resource> _resource, 
	D3D12_RESOURCE_STATES _before, 
	D3D12_RESOURCE_STATES _after)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_resource.Get(), _before, _after);

	_commandList->ResourceBarrier(1, &barrier);
}

void Lesson2::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList, 
	D3D12_CPU_DESCRIPTOR_HANDLE _handle, 
	FLOAT* _color)
{
	_commandList->ClearRenderTargetView(_handle, _color, 0, nullptr);
}

void Lesson2::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList, 
	D3D12_CPU_DESCRIPTOR_HANDLE _handle, 
	FLOAT _depth /*= 1.0f*/)
{
	_commandList->ClearDepthStencilView(_handle, D3D12_CLEAR_FLAG_DEPTH, _depth, 0, 0, nullptr);
}

//上传资源
void Lesson2::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList, 
	ID3D12Resource** _pDestinationResource, 
	ID3D12Resource** _pIntermediateResource, 
	size_t _numElements, 
	size_t _elementSize, 
	const void* _bufferData, 
	D3D12_RESOURCE_FLAGS _flags /*= D3D12_RESOURCE_FLAG_NONE*/)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = _numElements * _elementSize;

	auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto o = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, _flags);
	//CreateCommittedResource方法创建一个资源和一个足够大以存储该资源的隐式堆
	ThrowIfFailed(device->CreateCommittedResource(
		&p,
		D3D12_HEAP_FLAG_NONE,
		&o,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(_pDestinationResource)));

	if (_bufferData)
	{
		//创建一个上传堆，用于转交资源到默认堆
		p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		o = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&o,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(_pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subResource = {};
		subResource.pData = _bufferData;
		subResource.RowPitch = bufferSize;
		subResource.SlicePitch = subResource.RowPitch;

		//将上传堆中的资源上传至默认堆
		UpdateSubresources(_commandList.Get(), *_pDestinationResource, *_pIntermediateResource, 0, 0, 1, &subResource);
	}

	
}

void Lesson2::ResizeDepthBuffer(int _width, int _height)
{
	if (m_ContentLoaded)
	{
		//确保没有引用深度缓冲的命令在队列上执行
		Application::Get().Flush();

		_width = std::max(1, _width);
		_height = std::max(1, _height);

		auto device = Application::Get().GetDevice();

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.DepthStencil = { 1.0f, 0 };
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;

		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//用于创造D3D12_RESOURCE_DESC的辅助函数
		auto o = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _width, _height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&o,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_DepthBuffer)));

		//更新DSV
		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &desc, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

	}
	


}
