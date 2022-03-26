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
	//����Debug��
	static  void EnableDebufLayer();
	//������Ķ���
	static void ReportLiveObjects();

	//�����豸
	static std::shared_ptr<Device> Create(std::shared_ptr<Adapter> _adapter = nullptr);

	//��ȡ���ڴ����豸����������������Ϣ
	std::wstring GetDescription() const;

	//����ָ��������CPU�ɼ�������
	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptors = 1);

	//��ȡ�������Ĵ�С
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE _type) const
	{
		return m_d3d12Device->GetDescriptorHandleIncrementSize(_type);
	}

	//����һ��������ʹ���ṩ�Ĵ��ھ��
	std::shared_ptr<SwapChain> CreateSwapChain(HWND _hwnd, DXGI_FORMAT _backbufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM);

	//����һ��GUI
	std::shared_ptr<GUI> CreateGUI(HWND _hwnd, const RenderTarget& _rt);

    //���ø�������Դ����һ����������
    std::shared_ptr<ConstantBuffer> CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource);

    //����һ��λ��ַ����
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(size_t _bufferSize);
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource);

    //����һ���ṹ�建��
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(size_t _numElements, size_t _elementSize);
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        size_t _numElements, size_t _elementSize);

    /**
     * ����һ����ͼ��Դ
     *
     * @param resourceDesc ���ڴ�����ͼ��������Ϣ.
     * @param [clearVlue] �����ͼ�ĸ�ʽ
     * @param [textureUsage] ��ͼ���÷�
     *
     * @returns ����һ����������ͼ��ָ��
     */
    std::shared_ptr<Texture> CreateTexture(const D3D12_RESOURCE_DESC& _resourceDesc,
        const D3D12_CLEAR_VALUE* _clearValue = nullptr);
    std::shared_ptr<Texture> CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        const D3D12_CLEAR_VALUE* _clearValue = nullptr);

    //����һ����������
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat);
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numIndices,
        DXGI_FORMAT _indexFormat);

    //����һ�����㻺��
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(size_t _numVertices, size_t _vertexStride);
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> _resource,
        size_t _numVertices, size_t _vertexStride);

    //����һ����ǩ��
    //@param ��ǩ����������Ϣ
    std::shared_ptr<RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& _rootSignatureDesc);

    //����һ��pso
    //@param pso��������Ϣ
    template<class PipelineStateStream>
    std::shared_ptr<PipelineStateObject> CreatePipelineStateObject(PipelineStateStream& _pipelineStateStream)
    {
        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &_pipelineStateStream };

        return DoCreatePipelineStateObject(pipelineStateStreamDesc);
    }

    //����һ��CBV
    std::shared_ptr<ConstantBufferView> CreateConstantBufferView(const std::shared_ptr<ConstantBuffer>& _constantBuffer, size_t _offset = 0);

    //����SRV
    std::shared_ptr<ShaderResourceView> CreateShaderResourceView(const std::shared_ptr<Resource>& _resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv = nullptr);

    //����UAV
    std::shared_ptr<UnorderedAccessView> CreateUnorderedAccessView(
        const std::shared_ptr<Resource>& _resource,
        const std::shared_ptr<Resource>& _counterResource = nullptr,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav = nullptr);

    // ˢ�����е��������
    void Flush();

    //�ͷŹ�ʱ��������
    void ReleaseStaleDescriptors();

    //��ȡ���ڴ����豸��������
    std::shared_ptr<Adapter> GetAdapter() const
    {
        return m_Adapter;
    }

    /**
     * ��ȡһ��������У����Ե���������
     * - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
     * By default, a D3D12_COMMAND_LIST_TYPE_DIRECT queue is returned.
     */
    CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE _type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    //��ȡ�豸
    Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const
    {
        return m_d3d12Device;
    }

    //��ȡ��ߵĸ�ǩ���汾
    D3D_ROOT_SIGNATURE_VERSION GetHighestRootSignatureVersion() const
    {
        return m_HighestRootSignatureVersion;
    }

    //�����ز�����֧��
    DXGI_SAMPLE_DESC GetMultisampleQualityLevels(
        DXGI_FORMAT _format, UINT _numSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
        D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS _flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) const;

protected:
    explicit Device(std::shared_ptr<Adapter> _adapter);
    virtual ~Device();

    //����һ��PSO
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

