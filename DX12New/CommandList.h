#pragma once
#include <wrl.h>
#include <d3d12.h>

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "TextureUsage.hpp"
#include "RootSignature.h"

class Buffer;
class ByteAddressBuffer;
class ConstantBuffer;
class DynamicDescriptorHeap;
class GenerateMipsPSO;
class IndexBuffer;
class PanoToCubemapPSO;
class RenderTarget;
class Resource;
class ResourceStateTracker;
class StructuredBuffer;
class RootSignature;
class Texture;
class UploadBuffer;
class VertexBuffer;

class CommandList
{
public:
	CommandList(D3D12_COMMAND_LIST_TYPE);
	virtual ~CommandList();

	//��ȡ�����б������
	D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_CommandListType; }

	//��ȡ�б�
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const { return m_CommandList; }

	//����Դת�����ض�״̬
	//Ҫת������Դ
	//Ҫת����״̬
	//Ҫת��������Դ,Ĭ�������ת����������Դ
	//ǿ����������ϰ�
	void TransitionBarrier(const Resource& _resource, D3D12_RESOURCE_STATES _state, UINT _subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool _flushBarriers = false);
	void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, D3D12_RESOURCE_STATES _state, UINT _subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool _flushBarriers = false);

	//���һ��UAV����,ȷ������Դ��д���Ѿ����
	//Ҫת������Դ
	//ǿ����������ϰ�
	void UAVBarrier(const Resource& _resource, bool _flushBarriers = false);
	void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, bool _flushBarriers = false);

	//���һ����������
	//��ǰռ�öѵ���Դ
	//��Ҫռ�öѵ���Դ
	void AliasingBarrier(const Resource& _beforeResource, const Resource& _afterResource, bool _flushBarriers = false);
	void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> _afterResource, bool _flushBarriers = false);

	//ˢ���������͵������б����Դ����
	void FlushResourceBarriers();

	//����һ����Դ
	void CopyResource(Resource& _dstRes, const Resource& _srcRes);
	void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> _dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> _srcRes);

	//��һ�����ز�����Դ����Ϊ�Ƕ��ز�������Դ
	void ResolveSubresource(Resource& _detRes, const Resource& _srcRes, uint32_t _detSubresource = 0, uint32_t _srcSubresource = 0);

	//�����ݿ�����GPU�еĶ��㻺����
	void CopyVertexBuffer(VertexBuffer& _vertexBuffer, size_t _numVertices, size_t _vertexStride, const void* _vertexBufferData);
	template<typename T>
	void CopyVertexBuffer(VertexBuffer& _vertexBuffer, const std::vector<T>& _vertexBufferData)
	{
		CopyVertexBuffer(_vertexBuffer, _vertexBufferData.size(), sizeof(T), _vertexBufferData.data());
	}

	//�����ݿ�����GPU�е�����������
	void CopyIndexBuffer(IndexBuffer& _indexBuffer, size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexBufferData);
	template<typename T>
	void CopyIndexBuffer(IndexBuffer& _indexBuffer, const std::vector<T>& _indexBufferData)
	{
		assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		CopyIndexBuffer(_indexBuffer, _indexBufferData.size(), indexFormat, _indexBufferData.data());
	}

	//�����ݸ��Ƶ�GPU�е��ֽڵ�ַ������
	void CopyByteAddressBuffer(ByteAddressBuffer& _byteAddressBuffer, size_t _bufferSize, const void* _BufferData);
	template<typename T>
	void CopyByteAddressBuffer(ByteAddressBuffer& _byteAddressBuffer, const T& _BufferData)
	{
		CopyByteAddressBuffer(_byteAddressBuffer, sizeof(T), &_BufferData);
	}

	//�����ݸ��Ƶ�GPU�еĽṹ��������
	void CopyStructuredBuffer(StructuredBuffer& _structuredBuffer, size_t _numElements, size_t _elementSize, const void* _BufferData);
	template<typename T>
	void CopyStructuredBuffer(StructuredBuffer& _structuredBuffer, const std::vector<T>& _BufferData)
	{
		CopyStructuredBuffer(_structuredBuffer, _BufferData.size(), sizeof(T), _BufferData.data());
	}

	//����ͼԪ��������
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY _primitiveTopology);

	//���ļ��м���һֱ��ͼ
	void LoadTextureFromFile(Texture& _texture, const std::wstring& _fileName, TextureUsage _usage = TextureUsage::Albedo);

	//�������ݲ���ֱ�Ӹ��Ƶ�Ŀ��������Դ
	//��Ҫ���ϴ����д���һ���м仺����
	//�ֽ���Դ�������м仺������Ȼ���������б��Ϸ��������ݿ�����Ŀ���GPU����
	//ֻ��ִ�����������Դ�Żᱻ������Ŀ������
	//֮�����ʹ������
	void CopyTextureSubresource(Texture& _texture, uint32_t _firstSubresource, uint32_t _numSubresource, D3D12_SUBRESOURCE_DATA* _subresourceData);

	//���һ����ͼ
	void ClearTexture(const Texture& _texture, const float _clearColor[4]);

	//������ģ����ͼ
	void ClearDepthStencilTexture(const Texture& _texture, D3D12_CLEAR_FLAGS _clearFlags, float _depth = 1.0f, uint8_t _stencil = 0);

	//����MipMap
	void GenerateMips(Texture& _texture);

	//��һ��ȫ��ͼ����CUBEMAP(���ɵ�CUBEMAP�������ȫ��ͼ)
	void PanoToCubeMap(Texture& _cubeMap, const Texture& _pano);

	//����һ����̬����BUFFER����ǩ���е�����������
	//SetGraphics32BitConstants����Ҳ�������ڸ��¶�̬��������������,��ֻ�ʺ�С��16��32λ����������
	//���ڸ���ĳ���������,ʹ���ϴ��Ѹ���
	void SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, size_t _sizeInBytes, const void* _bufferData);
	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		SetGraphicsDynamicConstantBuffer(_rootParameterIndex, sizeof(T), &_bufferData);
	}

	//����һ��32λ��������Ⱦ����
	void SetGraphics32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData);
	template<typename T>
	void SetGraphics32BitConstants(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "���͵Ĵ�С������4���ֽڵı���");
		SetGraphics32BitConstants(_rootParameterIndex, sizeof(T) / sizeof(uint32_t), &_bufferData);
	}

	//����һ��32λ�������������
	void SetComputer32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData);
	template<typename T>
	void SetComputer32BitConstants(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "���͵Ĵ�С������4���ֽڵı���");
		SetComputer32BitConstants(_rootParameterIndex, sizeof(T) / sizeof(uint32_t), &_bufferData);
	}

	//���ö��㻺�嵽��Ⱦ����
	void SetVertexBuffer(uint32_t _slot, const VertexBuffer& _vertexBufffer);
	//����һ����̬���㻺�嵽��Ⱦ����
	void SetDynamicVertexBuffer(uint32_t _slot, size_t _numVertices, size_t _vertexSize, const void* _vertexBufferData);
	template<typename T>
	void SetDynamicVertexBuffer(uint32_t _slot, const std::vector<T>& _vertexBufferData)
	{
		SetDynamicVertexBuffer(_slot, _vertexBufferData.size(), sizeof(T), _vertexBufferData.data());
	}

	//�����������嵽��Ⱦ����
	void SetIndexBuffer(const IndexBuffer& _indexBuffer);
	//����һ����̬�������嵽��Ⱦ����
	void SetDynamicIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexxBufferData);
	template<typename T>
	void SetDynamicIndexBuffer(const std::vector<T>& _indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetDynamicIndexBuffer(_indexBufferData.size(), indexFormat, _indexBufferData.data());
	}

	//���ö�̬�ṹ������
	void SetGraphicsDynamicStructuredBuffer(uint32_t _slot, size_t _numElements, size_t _elementSize, const void* _bufferData);
	template<typename T>
	void SetGraphicsDynamicStructuredBuffer(uint32_t _slot, const std::vector<T>& _bufferData)
	{
		SetGraphicsDynamicStructuredBuffer(_slot, _bufferData.size(), sizeof(T), _bufferData.data());
	}

	//�����ӿ�
	void SetViewport(const D3D12_VIEWPORT& _viewport);
	void SetViewports(const std::vector<D3D12_VIEWPORT>& _viewports);

	//���ü���
	void SetScissorRect(const D3D12_RECT& _rect);
	void SetScissorRects(const std::vector<D3D12_RECT>& _rects);

	//����PSO
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pso);

	//���ø�ǩ��
	void SetGraphicsRootSignature(const dx12lib::RootSignature& _rootSignature);
	void SetComputerRootSignature(const dx12lib::RootSignature& _rootSignature);

	//����SRV
	//����������(��������������)
	//��������ƫ����
	//��Դ
	//��Ҫת������Դ״̬
	//����Դ����
	//����Դ����
	//SRV����
	void SetShaderResourceView(
		uint32_t _rootParameterIndex,
		uint32_t _descriptorOffset,
		const Resource& _resource,
		D3D12_RESOURCE_STATES _stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		UINT _firstSubresource = 0,
		UINT _numSubresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv = nullptr);

	//����UAV
	//����������
	//��������ƫ����
	//��Դ
	//��Ҫת������Դ״̬
	//����Դ����
	//����Դ����
	//UAV����
	void SetUnorderedAccessView(
		uint32_t _rootParameterIndex,
		uint32_t _descriptorOffset,
		const Resource& _resource,
		D3D12_RESOURCE_STATES _stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		UINT _firstSubresource = 0,
		UINT _numSubresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav = nullptr);

	//������ȾĿ��
	void SetRenderTarget(const RenderTarget& _renderTarget);

	//����ģ��
	void Draw(uint32_t _vertexCount, uint32_t _instanceCount = 1, uint32_t _startVertex = 0, uint32_t _startInstance = 0);
	void DrawIndexed(uint32_t _indexCount, uint32_t _instanceCount = 1, uint32_t _startIndex = 0, uint32_t _baseVertex = 0, uint32_t _startInstance = 0);

	//����һ������shader
	void Dispatch(uint32_t _numGroupsX, uint32_t _numGroupsY, uint32_t _numGroupsZ = 1);

	/***********************************************************************************
	����ĺ��������ڲ�ʹ��
	***********************************************************************************/
	//�ر������б�,
	//һ�����������й������Դ���ϵ������б�
	//����true��������Դ���϶�������
	bool Close(CommandList& _pendingCommandList);
	//�����ر������б�
	void Close();

	//�����������
	//Ӧ������������е���
	void Reset();

	//�ͷ�׷�ٵ�����,����������Ҫ���ô�Сʱʹ��
	void ReleaseTrackedObjects();

	//������������
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, ID3D12DescriptorHeap* _heap);

	std::shared_ptr<CommandList> GetGenerateMipsCommandList() const { return m_ComputerCommandList; }

private:
	void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> _object);
	void TrackResource(const Resource& _res);

	//����mips
	void GenerateMips_UAV(Texture& _texture, DXGI_FORMAT _format);
	void GenerateMips_BGR(Texture& _texture);
	void GenerateMips_sRGB(Texture& _texture);

	//�����ݴ�CPU������GPU����
	void CopyBuffer(Buffer& _buffer, size_t _numElements, size_t _elementSize, const void* _bufferData, D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE);

	//����ǰ���������Ѱ󶨵������б�
	void BindDescriptorHeaps();

	//ID3D12Object��VECTOR����
	using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

	//�б������
	D3D12_COMMAND_LIST_TYPE m_CommandListType;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	//���ƶ��п�����Ҫ�ڼ�������ʱ����Mips,�����ƶ����޷�����Mips
	//���,�ڸ��ƶ����������Դ���ϴ���,�����ɲ�ִ�м����������
	std::shared_ptr<CommandList> m_ComputerCommandList;

	//��ǩ��
	ID3D12RootSignature* m_RootSignature = nullptr;

	//��Դ�������ϴ�����,���ڻ��ƶ�̬��ģ��
	//�����ϴ���Щÿ֡���ᱻ�ı�ĳ�����������
	std::unique_ptr<UploadBuffer> m_UploadBuffer = nullptr;

	//��Դ״̬׷����,�������б�����׷�ٵ�ǰ����Դ״̬
	//ͬʱҲ׷��ȫ����Դ״̬
	std::unique_ptr<ResourceStateTracker> m_ResourceStateTrack = nullptr;

	//��̬��������
	//��Ҫ���ύ���������ǰ����֯���
	std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//׷�ٵ�ǰ�󶨵���������,��������뵱ǰ�󶨵��������Ѳ�ͬ,������������
	ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//pso
	std::unique_ptr<PanoToCubemapPSO> m_PanoToCubemapPSO = nullptr;
	std::unique_ptr<GenerateMipsPSO> m_GenerateMipsPSO = nullptr;

	//�������б�׷�ٵ��������е���������ϵ�����,���ܱ�ɾ��
	//ȷ�����б�ִ�����֮ǰ����ɾ��
	//����������ô洢����
	//�������б�����ʱ���ͷ�
	TrackedObjects m_TrackedObjects;

	//׷���Ѿ����ص���ͼ,ȷ���������ͬһ����ͼ���
	static std::map<std::wstring, ID3D12Resource*> ms_TextureCache;
	static std::mutex ms_TextureCacheMutex;
};
