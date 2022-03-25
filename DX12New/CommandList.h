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

	//获取命令列表的类型
	D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_CommandListType; }

	//获取列表
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const { return m_CommandList; }

	//将资源转换到特定状态
	//要转换的资源
	//要转换的状态
	//要转换的子资源,默认情况下转换所有子资源
	//强制清除所有障碍
	void TransitionBarrier(const Resource& _resource, D3D12_RESOURCE_STATES _state, UINT _subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool _flushBarriers = false);
	void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, D3D12_RESOURCE_STATES _state, UINT _subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool _flushBarriers = false);

	//添加一个UAV屏障,确保对资源的写入已经完成
	//要转换的资源
	//强制清除所有障碍
	void UAVBarrier(const Resource& _resource, bool _flushBarriers = false);
	void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _resource, bool _flushBarriers = false);

	//添加一个别名屏障
	//当前占用堆的资源
	//将要占用堆的资源
	void AliasingBarrier(const Resource& _beforeResource, const Resource& _afterResource, bool _flushBarriers = false);
	void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> _beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> _afterResource, bool _flushBarriers = false);

	//刷新所有推送到命令列表的资源屏障
	void FlushResourceBarriers();

	//复制一个资源
	void CopyResource(Resource& _dstRes, const Resource& _srcRes);
	void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> _dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> _srcRes);

	//将一个多重采样资源解析为非多重采样的资源
	void ResolveSubresource(Resource& _detRes, const Resource& _srcRes, uint32_t _detSubresource = 0, uint32_t _srcSubresource = 0);

	//将内容拷贝到GPU中的顶点缓冲区
	void CopyVertexBuffer(VertexBuffer& _vertexBuffer, size_t _numVertices, size_t _vertexStride, const void* _vertexBufferData);
	template<typename T>
	void CopyVertexBuffer(VertexBuffer& _vertexBuffer, const std::vector<T>& _vertexBufferData)
	{
		CopyVertexBuffer(_vertexBuffer, _vertexBufferData.size(), sizeof(T), _vertexBufferData.data());
	}

	//将内容拷贝到GPU中的索引缓冲区
	void CopyIndexBuffer(IndexBuffer& _indexBuffer, size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexBufferData);
	template<typename T>
	void CopyIndexBuffer(IndexBuffer& _indexBuffer, const std::vector<T>& _indexBufferData)
	{
		assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		CopyIndexBuffer(_indexBuffer, _indexBufferData.size(), indexFormat, _indexBufferData.data());
	}

	//将内容复制到GPU中的字节地址缓冲区
	void CopyByteAddressBuffer(ByteAddressBuffer& _byteAddressBuffer, size_t _bufferSize, const void* _BufferData);
	template<typename T>
	void CopyByteAddressBuffer(ByteAddressBuffer& _byteAddressBuffer, const T& _BufferData)
	{
		CopyByteAddressBuffer(_byteAddressBuffer, sizeof(T), &_BufferData);
	}

	//将内容复制到GPU中的结构化缓冲区
	void CopyStructuredBuffer(StructuredBuffer& _structuredBuffer, size_t _numElements, size_t _elementSize, const void* _BufferData);
	template<typename T>
	void CopyStructuredBuffer(StructuredBuffer& _structuredBuffer, const std::vector<T>& _BufferData)
	{
		CopyStructuredBuffer(_structuredBuffer, _BufferData.size(), sizeof(T), _BufferData.data());
	}

	//设置图元拓扑类型
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY _primitiveTopology);

	//从文件中加载一直贴图
	void LoadTextureFromFile(Texture& _texture, const std::wstring& _fileName, TextureUsage _usage = TextureUsage::Albedo);

	//像素数据不能直接复制到目标纹理资源
	//需要在上传堆中创建一个中间缓冲区
	//现将资源拷贝到中间缓冲区，然后再命令列表上发出将数据拷贝到目标的GPU命令
	//只有执行了命令后资源才会被拷贝到目标区域
	//之后才能使用纹理
	void CopyTextureSubresource(Texture& _texture, uint32_t _firstSubresource, uint32_t _numSubresource, D3D12_SUBRESOURCE_DATA* _subresourceData);

	//清空一张贴图
	void ClearTexture(const Texture& _texture, const float _clearColor[4]);

	//清空深度模板贴图
	void ClearDepthStencilTexture(const Texture& _texture, D3D12_CLEAR_FLAGS _clearFlags, float _depth = 1.0f, uint8_t _stencil = 0);

	//生成MipMap
	void GenerateMips(Texture& _texture);

	//从一张全景图生成CUBEMAP(生成的CUBEMAP，传入的全景图)
	void PanoToCubeMap(Texture& _cubeMap, const Texture& _pano);

	//设置一个动态常量BUFFER到根签名中的内联描述符
	//SetGraphics32BitConstants函数也可以用于更新动态常量缓冲区数据,但只适合小于16个32位常量的数据
	//对于更大的常量缓冲区,使用上传堆更好
	void SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, size_t _sizeInBytes, const void* _bufferData);
	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		SetGraphicsDynamicConstantBuffer(_rootParameterIndex, sizeof(T), &_bufferData);
	}

	//设置一个32位常量到渲染管线
	void SetGraphics32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData);
	template<typename T>
	void SetGraphics32BitConstants(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "类型的大小必须是4个字节的倍数");
		SetGraphics32BitConstants(_rootParameterIndex, sizeof(T) / sizeof(uint32_t), &_bufferData);
	}

	//设置一个32位常量到计算管线
	void SetComputer32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData);
	template<typename T>
	void SetComputer32BitConstants(uint32_t _rootParameterIndex, const T& _bufferData)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "类型的大小必须是4个字节的倍数");
		SetComputer32BitConstants(_rootParameterIndex, sizeof(T) / sizeof(uint32_t), &_bufferData);
	}

	//设置顶点缓冲到渲染管线
	void SetVertexBuffer(uint32_t _slot, const VertexBuffer& _vertexBufffer);
	//设置一个动态顶点缓冲到渲染管线
	void SetDynamicVertexBuffer(uint32_t _slot, size_t _numVertices, size_t _vertexSize, const void* _vertexBufferData);
	template<typename T>
	void SetDynamicVertexBuffer(uint32_t _slot, const std::vector<T>& _vertexBufferData)
	{
		SetDynamicVertexBuffer(_slot, _vertexBufferData.size(), sizeof(T), _vertexBufferData.data());
	}

	//设置索引缓冲到渲染管线
	void SetIndexBuffer(const IndexBuffer& _indexBuffer);
	//设置一个动态索引缓冲到渲染管线
	void SetDynamicIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexxBufferData);
	template<typename T>
	void SetDynamicIndexBuffer(const std::vector<T>& _indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetDynamicIndexBuffer(_indexBufferData.size(), indexFormat, _indexBufferData.data());
	}

	//设置动态结构化缓冲
	void SetGraphicsDynamicStructuredBuffer(uint32_t _slot, size_t _numElements, size_t _elementSize, const void* _bufferData);
	template<typename T>
	void SetGraphicsDynamicStructuredBuffer(uint32_t _slot, const std::vector<T>& _bufferData)
	{
		SetGraphicsDynamicStructuredBuffer(_slot, _bufferData.size(), sizeof(T), _bufferData.data());
	}

	//设置视口
	void SetViewport(const D3D12_VIEWPORT& _viewport);
	void SetViewports(const std::vector<D3D12_VIEWPORT>& _viewports);

	//设置剪裁
	void SetScissorRect(const D3D12_RECT& _rect);
	void SetScissorRects(const std::vector<D3D12_RECT>& _rects);

	//设置PSO
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pso);

	//设置根签名
	void SetGraphicsRootSignature(const dx12lib::RootSignature& _rootSignature);
	void SetComputerRootSignature(const dx12lib::RootSignature& _rootSignature);

	//设置SRV
	//根参数索引(必须是描述符表)
	//描述符的偏移量
	//资源
	//需要转换的资源状态
	//子资源索引
	//子资源数量
	//SRV描述
	void SetShaderResourceView(
		uint32_t _rootParameterIndex,
		uint32_t _descriptorOffset,
		const Resource& _resource,
		D3D12_RESOURCE_STATES _stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		UINT _firstSubresource = 0,
		UINT _numSubresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv = nullptr);

	//设置UAV
	//根参数索引
	//描述符的偏移量
	//资源
	//需要转换的资源状态
	//子资源索引
	//子资源数量
	//UAV描述
	void SetUnorderedAccessView(
		uint32_t _rootParameterIndex,
		uint32_t _descriptorOffset,
		const Resource& _resource,
		D3D12_RESOURCE_STATES _stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		UINT _firstSubresource = 0,
		UINT _numSubresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav = nullptr);

	//设置渲染目标
	void SetRenderTarget(const RenderTarget& _renderTarget);

	//绘制模型
	void Draw(uint32_t _vertexCount, uint32_t _instanceCount = 1, uint32_t _startVertex = 0, uint32_t _startInstance = 0);
	void DrawIndexed(uint32_t _indexCount, uint32_t _instanceCount = 1, uint32_t _startIndex = 0, uint32_t _baseVertex = 0, uint32_t _startInstance = 0);

	//调用一个计算shader
	void Dispatch(uint32_t _numGroupsX, uint32_t _numGroupsY, uint32_t _numGroupsZ = 1);

	/***********************************************************************************
	下面的函数仅供内部使用
	***********************************************************************************/
	//关闭命令列表,
	//一个包含了所有挂起的资源屏障的命令列表
	//返回true当所有资源屏障都被处理
	bool Close(CommandList& _pendingCommandList);
	//仅仅关闭命令列表
	void Close();

	//重置命令队列
	//应当仅被命令队列调用
	void Reset();

	//释放追踪的物体,当交换链需要重置大小时使用
	void ReleaseTrackedObjects();

	//设置描述符堆
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, ID3D12DescriptorHeap* _heap);

	std::shared_ptr<CommandList> GetGenerateMipsCommandList() const { return m_ComputerCommandList; }

private:
	void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> _object);
	void TrackResource(const Resource& _res);

	//生成mips
	void GenerateMips_UAV(Texture& _texture, DXGI_FORMAT _format);
	void GenerateMips_BGR(Texture& _texture);
	void GenerateMips_sRGB(Texture& _texture);

	//将内容从CPU拷贝到GPU当中
	void CopyBuffer(Buffer& _buffer, size_t _numElements, size_t _elementSize, const void* _bufferData, D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE);

	//将当前的描述符堆绑定到命令列表
	void BindDescriptorHeaps();

	//ID3D12Object的VECTOR容器
	using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

	//列表的类型
	D3D12_COMMAND_LIST_TYPE m_CommandListType;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	//复制队列可能需要在加载纹理时生成Mips,但复制队列无法生成Mips
	//因此,在复制队列完成子资源的上传后,将生成并执行计算命令队列
	std::shared_ptr<CommandList> m_ComputerCommandList;

	//根签名
	ID3D12RootSignature* m_RootSignature = nullptr;

	//资源创建在上传堆上,用于绘制动态的模型
	//或者上传那些每帧都会被改变的常量缓冲数据
	std::unique_ptr<UploadBuffer> m_UploadBuffer = nullptr;

	//资源状态追踪器,被命令列表用于追踪当前的资源状态
	//同时也追踪全局资源状态
	std::unique_ptr<ResourceStateTracker> m_ResourceStateTrack = nullptr;

	//动态描述符堆
	//需要在提交到命令队列前被组织完成
	std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//追踪当前绑定的描述符堆,如果它们与当前绑定的描述符堆不同,更改描述符堆
	ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//pso
	std::unique_ptr<PanoToCubemapPSO> m_PanoToCubemapPSO = nullptr;
	std::unique_ptr<GenerateMipsPSO> m_GenerateMipsPSO = nullptr;

	//由命令列表追踪的在运行中的命令队列上的物体,不能被删除
	//确保在列表执行完成之前不被删除
	//将对象的引用存储起来
	//在命令列表重置时被释放
	TrackedObjects m_TrackedObjects;

	//追踪已经加载的贴图,确保不会加载同一张贴图多次
	static std::map<std::wstring, ID3D12Resource*> ms_TextureCache;
	static std::mutex ms_TextureCacheMutex;
};
