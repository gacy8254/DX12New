#include "Device.h"
#include "Adapter.h"
#include "helpers.h"
#include "D3D12LibPCH.h"
#include <dxgidebug.h>
#include "DescriptorAllocator.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "GUI.h"
#include "ConstantBuffer.h"
#include "ByteAddressBuffer.h"

class MakeDevice : public Device
{
public:
	MakeDevice(std::shared_ptr<Adapter> _adapter) :Device(_adapter) {}

	virtual ~MakeDevice() {}
};

class MakeCommandQueue :public CommandQueue
{
public:
	MakeCommandQueue(Device& _device , D3D12_COMMAND_LIST_TYPE _type)
		:CommandQueue(_device, _type){}

	virtual ~MakeCommandQueue();
};

class MakeDescriptorAllocator : public DescriptorAllocator
{
public:
	MakeDescriptorAllocator(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256)
		: DescriptorAllocator(device, type, numDescriptorsPerHeap)
	{}

	virtual ~MakeDescriptorAllocator() {}
};

class MakeSwapChain : public SwapChain
{
public:
	MakeSwapChain(Device& device, HWND hWnd, DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM)
		: SwapChain(device, hWnd, backBufferFormat)
	{}

	virtual ~MakeSwapChain() {}
};

class MakeConstantBuffer : public ConstantBuffer
{
public:
	MakeConstantBuffer(Device& device, ComPtr<ID3D12Resource> resource)
		: ConstantBuffer(device, resource)
	{}

	virtual ~MakeConstantBuffer() {}
};

class MakeByteAddressBuffer : public ByteAddressBuffer
{
public:
	MakeByteAddressBuffer(Device& device, const D3D12_RESOURCE_DESC& desc)
		: ByteAddressBuffer(device, desc)
	{}

	MakeByteAddressBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resoruce)
		: ByteAddressBuffer(device, resoruce)
	{}

	virtual ~MakeByteAddressBuffer() {}
};

void Device::EnableDebufLayer()
{
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
}

void Device::ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

std::shared_ptr<Device> Device::Create(std::shared_ptr<Adapter> _adapter /*= nullptr*/)
{
	return std::make_shared<MakeDevice>(_adapter);
}

std::wstring Device::GetDescription() const
{
	return m_Adapter->GetDescriptor();
}

DescriptorAllocation Device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptors /*= 1*/)
{
	return m_DescriptorAlloctor[_type]->Allocate(_numDescriptors);
}

std::shared_ptr<SwapChain> Device::CreateSwapChain(HWND _hwnd, DXGI_FORMAT _backbufferFormat /*= DXGI_FORMAT_R10G10B10A2_UNORM*/)
{
	std::shared_ptr<SwapChain> swapChain;
	swapChain = std::make_shared<MakeSwapChain>(*this, _hwnd, _backbufferFormat);

	return swapChain;
}

std::shared_ptr<GUI> Device::CreateGUI(HWND _hwnd, const RenderTarget& _rt)
{
	std::shared_ptr<GUI> gui = std::make_shared<GUI>();

	return gui;
}

std::shared_ptr<ConstantBuffer> Device::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource)
{
	std::shared_ptr<ConstantBuffer> constantBuffer = std::make_shared<MakeConstantBuffer>(*this, _resource);

	return constantBuffer;
}

std::shared_ptr<ByteAddressBuffer> Device::CreateByteAddressBuffer(size_t _bufferSize)
{
	_bufferSize = Math::AlignUp(_bufferSize, 4);

	std::shared_ptr<ByteAddressBuffer> Buffer = std::make_shared<MakeByteAddressBuffer>(*this, CD3DX12_RESOURCE_DESC::Buffer(_bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));

	return Buffer;
}

std::shared_ptr<ByteAddressBuffer> Device::CreateByteAddressBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource)
{

}

std::shared_ptr<StructuredBuffer> Device::CreateStructuredBuffer(size_t _numElements, size_t _elementSize)
{

}

std::shared_ptr<StructuredBuffer> Device::CreateStructuredBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numElements, size_t _elementSize)
{

}

std::shared_ptr<Texture> Device::CreateTexture(const D3D12_RESOURCE_DESC& _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
{

}

std::shared_ptr<Texture> Device::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
{

}

std::shared_ptr<IndexBuffer> Device::CreateIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat)
{

}

std::shared_ptr<IndexBuffer> Device::CreateIndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numIndices, DXGI_FORMAT _indexFormat)
{

}

std::shared_ptr<VertexBuffer> Device::CreateVertexBuffer(size_t _numVertices, size_t _vertexStride)
{

}

std::shared_ptr<VertexBuffer> Device::CreateVertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numVertices, size_t _vertexStride)
{

}

std::shared_ptr<dx12lib::RootSignature> Device::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc)
{

}

std::shared_ptr<ConstantBufferView> Device::CreateConstantBufferView(const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t _offset /*= 0*/)
{

}

std::shared_ptr<ShaderResourceView> Device::CreateShaderResourceView(const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv /*= nullptr*/)
{

}

std::shared_ptr<UnorderedAccessView> Device::CreateUnorderedAccessView(const std::shared_ptr<Resource>& _resource, 
	const std::shared_ptr<Resource>& _counterResource /*= nullptr*/, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav /*= nullptr*/)
{

}

void Device::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

void Device::ReleaseStaleDescriptors()
{
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	{
		m_DescriptorAlloctor[i]->ReleaseStaleDescriptor();
	}
}

CommandQueue& Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type /*= D3D12_COMMAND_LIST_TYPE_DIRECT*/)
{
	CommandQueue* commandQueue = nullptr;
	switch (_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue.get();
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue.get();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue.get();
		break;
	default:
		assert(false && "无效的类型");
		break;
	}

	return *commandQueue;
}

DXGI_SAMPLE_DESC Device::GetMultisampleQualityLevels(DXGI_FORMAT _format, 
	UINT _numSamples /*= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT*/, 
	D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS _flags /*= D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE*/) const
{

}

Device::Device(std::shared_ptr<Adapter> _adapter)
	:m_Adapter(_adapter)
{
	if (!m_Adapter)
	{
		m_Adapter = Adapter::Create();
		assert(m_Adapter);
	}

	auto dxgiAdapter = m_Adapter->GetDXGIAdapter();

	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

	//开启DEBUG消息
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		//设置消息过滤
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO                                     //目前没有消息根据类别被忽略
		};

		D3D12_MESSAGE_ID DenyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(severities);
		NewFilter.DenyList.pSeverityList = severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif


	m_DirectCommandQueue = std::make_unique<MakeCommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = std::make_unique<MakeCommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = std::make_unique<MakeCommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COPY);

	//创建描述符分配器
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorAlloctor[i] = std::make_unique<MakeDescriptorAllocator>(*this, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}

	//检查功能支持
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,
		sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	m_HighestRootSignatureVersion = featureData.HighestVersion;
}

Device::~Device(){}

std::shared_ptr<PipelineStateObject> Device::DoCreatePipelineStateObject(const D3D12_PIPELINE_STATE_STREAM_DESC& pipelineStateStreamDesc)
{

}
