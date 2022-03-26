#pragma once

#include "DescriptorAllocation.h"

#include "d3dx12.h"
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <memory>
#include <string>
#include "RootSignature.h"

class Adapter;
class ByteAddressBuffer;
class CommandQueue;
class CommandList;
class ConstantBuffer;
class ConstantBufferView;
class DescriptorAllocator;
class GUI;
class IndexBuffer;
class PipelineStateObject;
class RenderTarget;
class Resource;
class Scene;
class ShaderResourceView;
class StructuredBuffer;
class SwapChain;
class Texture;
class UnorderedAccessView;
class VertexBuffer;

class Device
{
public:
	//开关Debug层
	static  void EnableDebufLayer();
	//报告存活的对象
	static void ReportLiveObjects();

	//创建设备
	static std::shared_ptr<Device> Create(std::shared_ptr<Adapter> _adapter = nullptr);

	//获取用于创建设备的适配器的描述信息
	std::wstring GetDescription() const;

	//分配指定数量的CPU可见描述符
	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptors = 1);

	//获取描述符的大小
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE _type) const
	{
		return m_d3d12Device->GetDescriptorHandleIncrementSize(_type);
	}

	//创建一个交换链使用提供的窗口句柄
	std::shared_ptr<SwapChain> CreateSwapChain(HWND _hwnd, DXGI_FORMAT _backbufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM);

	//创建一个GUI
	std::shared_ptr<GUI> CreateGUI(HWND _hwnd, const RenderTarget& _rt);

    //利用给定的资源创建一个常量缓冲
    std::shared_ptr<ConstantBuffer> CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource);

    //创建一个位地址缓冲
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(size_t _bufferSize);
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource);

    //创建一个结构体缓冲
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(size_t _numElements, size_t _elementSize);
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        size_t _numElements, size_t _elementSize);

    /**
     * 创建一个贴图资源
     *
     * @param resourceDesc 用于创建贴图的描述信息.
     * @param [clearVlue] 清空贴图的格式
     * @param [textureUsage] 贴图的用法
     *
     * @returns 返回一个创建的贴图的指针
     */
    std::shared_ptr<Texture> CreateTexture(const D3D12_RESOURCE_DESC& _resourceDesc,
        const D3D12_CLEAR_VALUE* _clearValue = nullptr);
    std::shared_ptr<Texture> CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        const D3D12_CLEAR_VALUE* _clearValue = nullptr);

    //创建一个索引缓冲
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat);
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numIndices,
        DXGI_FORMAT _indexFormat);

    //创建一个顶点缓冲
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(size_t _numVertices, size_t _vertexStride);
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        size_t _numVertices, size_t _vertexStride);

    //创建一个根签名
    //@param 根签名的描述信息
    std::shared_ptr<RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc);

    //创建一个pso
    //@param pso的描述信息
    template<class PipelineStateStream>
    std::shared_ptr<PipelineStateObject> CreatePipelineStateObject(PipelineStateStream& _pipelineStateStream)
    {
        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &_pipelineStateStream };

        return DoCreatePipelineStateObject(pipelineStateStreamDesc);
    }

    //创建一个CBV
    std::shared_ptr<ConstantBufferView> CreateConstantBufferView(const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t _offset = 0);

    //创建SRV
    std::shared_ptr<ShaderResourceView> CreateShaderResourceView(const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv = nullptr);

    //创建UAV
    std::shared_ptr<UnorderedAccessView> CreateUnorderedAccessView(
        const std::shared_ptr<Resource>& _resource,
        const std::shared_ptr<Resource>& _counterResource = nullptr,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav = nullptr);

    // 刷新所有的命令队列
    void Flush();

    //释放过时的描述符
    void ReleaseStaleDescriptors();

    //获取用于创建设备的适配器
    std::shared_ptr<Adapter> GetAdapter() const
    {
        return m_Adapter;
    }

    /**
     * 获取一个命令队列，可以的类型如下
     * - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
     * By default, a D3D12_COMMAND_LIST_TYPE_DIRECT queue is returned.
     */
    CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    //获取设备
    Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const
    {
        return m_d3d12Device;
    }

    //获取最高的根签名版本
    D3D_ROOT_SIGNATURE_VERSION GetHighestRootSignatureVersion() const
    {
        return m_HighestRootSignatureVersion;
    }

    //检查多重采样的支持
    DXGI_SAMPLE_DESC GetMultisampleQualityLevels(
        DXGI_FORMAT _format, UINT _numSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
        D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS _flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) const;

protected:
    explicit Device(std::shared_ptr<Adapter> _adapter);
    virtual ~Device();

    //创建一个PSO
    std::shared_ptr<PipelineStateObject> DoCreatePipelineStateObject(const D3D12_PIPELINE_STATE_STREAM_DESC& pipelineStateStreamDesc);

private:
	Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

	std::shared_ptr<Adapter> m_Adapter;

	std::shared_ptr<CommandQueue> m_DirectCommandQueue;
	std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
	std::shared_ptr<CommandQueue> m_CopyCommandQueue;

	std::unique_ptr<DescriptorAllocator> m_DescriptorAlloctor[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D_ROOT_SIGNATURE_VERSION m_HighestRootSignatureVersion;
};

