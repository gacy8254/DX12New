#include "CommandList.h"
#include "Application.h"
#include "ByteAddressBuffer.h"
#include "ConstantBuffer.h"
#include "CommandQueue.h"
//#include "GenerateMipsPSO.h"
#include "IndexBuffer.h"
//#include "PanoToCubemapPSO.h"
#include "RenderTarget.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "RootSignature.h"
#include "StructuredBuffer.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexBuffer.h"
#include "DynamicDescriptorHeap.h"

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE)
{

}

CommandList::~CommandList()
{

}

void CommandList::TransitionBarrier(const Resource& _resource, D3D12_RESOURCE_STATES _state, UINT _subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, bool _flushBarriers /*= false*/)
{
	//获取资源
	//创建一个资源屏障
	//交由资源屏障追踪类进行处理
	auto resource = _resource.GetResource();
	if (resource)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, _state, _subresource);
		m_ResourceStateTrack->ResourceBarrier(barrier);
	}

	//如果资源屏障需要提交到命令列表,则刷新资源屏障
	//该方法调用ResourceStateTracker::FlushResourceBarriers函数
	if (_flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const Resource& _resource, bool _flushBarriers /*= false*/)
{

}

void CommandList::AliasingBarrier(const Resource& _beforeResource, const Resource& _afterResource, bool _flushBarriers /*= false*/)
{

}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTrack->FlushResourceBarrier(*this);
}

void CommandList::CopyResource(Resource& _dstRes, const Resource& _srcRes)
{
	//分别将目标和来源资源转换到相应的状态
	TransitionBarrier(_dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(_srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	//执行资源转换
	FlushResourceBarriers();

	//复制资源
	m_CommandList->CopyResource(_dstRes.GetResource().Get(), _srcRes.GetResource().Get());

	//追踪资源
	TrackResource(_dstRes);
	TrackResource(_srcRes);
}

void CommandList::ResolveSubresource(Resource& _detRes, const Resource& _srcRes, uint32_t _detSubresource /*= 0*/, uint32_t _srcSubresource /*= 0*/)
{

}

void CommandList::CopyVertexBuffer(VertexBuffer& _vertexBuffer, size_t _numVertices, size_t _vertexStride, const void* _vertexBufferData)
{

}

void CommandList::CopyIndexBuffer(IndexBuffer& _indexBuffer, size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexBufferData)
{

}

void CommandList::CopyByteAddressBuffer(ByteAddressBuffer& _byteAddressBuffer, size_t _bufferSize, const void* _BufferData)
{

}

void CommandList::CopStructuredBuffer(StructuredBuffer& _structuredBuffer, size_t _numElements, size_t _elementSize, const void* _BufferData)
{

}

void CommandList::StePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY _primitiveTopology)
{

}

void CommandList::LoadTextureFromFile(Texture& _texture, const std::wstring& _fileName)
{

}

void CommandList::ClearTexture(const Texture& _texture, const float _clearColor[4])
{

}

void CommandList::ClearDepthStencilTexture(const Texture& _texture, D3D12_CLEAR_FLAGS _clearFlags, float _depth /*= 1.0f*/, uint8_t _stencil /*= 0*/)
{

}

void CommandList::GenerateMips(Texture& _texture)
{

}

void CommandList::PanoToCubeMap(Texture& _cubeMap, const Texture& _pano)
{

}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t _rootParameterIndex, size_t _sizeInBytes, const void* _bufferData)
{
	//常量缓存必须256位对齐
	//使用UploadBuffer类分配内存
	//用于更新经常更改的常量缓冲区
	//Allocate方法返回一个ALLOCATION结构体,仅包括指向上传堆中内存的CPU和GPU指针
	auto heapAllocation = m_UploadBuffer->Allocate(_sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	//拷贝内存
	memcpy(heapAllocation.CPU, _bufferData, _sizeInBytes);

	//设置CBV
	m_CommandList->SetGraphicsRootConstantBufferView(_rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetGraphics32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData)
{

}

void CommandList::SetComputer32BitConstants(uint32_t _rootParameterIndex, size_t _numContants, const void* _bufferData)
{

}

void CommandList::SetDynamicVertexBuffer(uint32_t _slot, size_t _numVertices, size_t _vertexSize, const void* _vertexBufferData)
{

}

void CommandList::SetVertexBuffer(uint32_t _slot, const VertexBuffer& _vertexBufffer)
{

}

void CommandList::SetVertexBuffer(const IndexBuffer& _indexBuffer)
{

}

void CommandList::SetDynamicIndexBuffer(size_t _numIndicies, DXGI_FORMAT _indexFormat, const void* _indexxBufferData)
{

}

void CommandList::SetGraphicsDynamicStructuredBuffer(uint32_t _slot, size_t _numElements, size_t _elementSize, const void* _bufferData)
{

}

void CommandList::SetViewport(const D3D12_VIEWPORT& _viewport)
{

}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& _viewports)
{

}

void CommandList::SetScissorRect(const D3D12_RECT& _rect)
{

}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& _rects)
{

}

void CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pso)
{

}

void CommandList::SetGraphicsRootSignature(const RootSignature& _rootSignature)
{

}

void CommandList::SetComputerRootSignature(const RootSignature& _rootSignature)
{

}

void CommandList::SetShaderResourceView(
	uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const Resource& _resource, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC* _srv /*= nullptr*/)
{
	//先转换资源的状态
	if (_numSubresource < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < _numSubresource; ++i)
		{
			TransitionBarrier(_resource, _stateAfter, _firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(_resource, _stateAfter);
	}

	//暂存资源
	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _resource.GetShaderResourceView(_srv));

	//追踪资源
	TrackResource(_resource);
}

void CommandList::SetUnorderedAccessView(uint32_t _rootParameterIndex, 
	uint32_t _descriptorOffset, 
	const Resource& _resource, 
	D3D12_RESOURCE_STATES _stateAfter /*= D3D12_RESOURCE_STATE_UNORDERED_ACCESS*/, 
	UINT _firstSubresource /*= 0*/, 
	UINT _numSubresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* _uav /*= nullptr*/)
{
	//先转换资源的状态
	if (_numSubresource < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < _numSubresource; ++i)
		{
			TransitionBarrier(_resource, _stateAfter, _firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(_resource, _stateAfter);
	}

	//暂存资源
	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptor(_rootParameterIndex, _descriptorOffset, 1, _resource.GetUnorderedAccessView(_uav));

	//追踪资源
	TrackResource(_resource);
}

void CommandList::SetRenderTarget(const RenderTarget& _renderTarget)
{

}

void CommandList::Draw(uint32_t _vertexCount, uint32_t _instanceCount /*= 1*/, uint32_t _startVertex /*= 0*/, uint32_t _startInstance /*= 0*/)
{
	//完成资源转换
	FlushResourceBarriers();

	//提交所有描述符堆的资源
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	//绘制
	m_CommandList->DrawInstanced(_vertexCount, _instanceCount, _startVertex, _startInstance);
}

void CommandList::DrawIndexed(
	uint32_t _indexCount, 
	uint32_t _instancesCount /*= 1*/, 
	uint32_t _startIndex /*= 0*/, 
	uint32_t _baseVertex /*= 0*/, 
	uint32_t _startInstance /*= 0*/)
{
	//完成资源转换
	FlushResourceBarriers();

	//提交所有描述符堆的资源
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	//绘制
	m_CommandList->DrawIndexedInstanced(_indexCount, _instancesCount, _startIndex, _baseVertex, _startInstance);
}

void CommandList::Dispatch(uint32_t _numGroupsX, uint32_t _numGroupsY, uint32_t _numGroupsZ /*= 1*/)
{

}

bool CommandList::Close(CommandList& _pendingCommandList)
{
	bool b = true;

	return b;
}

void CommandList::Close()
{

}

void CommandList::Reset()
{

}

void CommandList::ReleaseTrackedObjects()
{

}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE _type, ID3D12DescriptorHeap* _heap)
{
	if (m_DescriptorHeaps[_type] != _heap)
	{
		m_DescriptorHeaps[_type] = _heap;
		BindDescriptorHeaps();
	}
}

void CommandList::TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> _object)
{
	m_TrackedObjects.push_back(_object);
}

void CommandList::TrackResource(const Resource& _res)
{
	TrackObject(_res.GetResource());
}

void CommandList::GenerateMips_UAV(Texture& _texture)
{

}

void CommandList::GenerateMips_BGR(Texture& _texture)
{

}

void CommandList::GenerateMips_sRGB(Texture& _texture)
{

}

void CommandList::CopyBuffer(Buffer& _buffer, size_t _numElements, size_t _elementSize, const void* _bufferData, D3D12_RESOURCE_FLAGS _flags /*= D3D12_RESOURCE_FLAG_NONE*/)
{

}

void CommandList::BindDescriptorHeaps()
{
	UINT numDescriptor = 0;
	ID3D12DescriptorHeap* desHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
		if (descriptorHeap)
		{
			desHeap[numDescriptor++] = descriptorHeap;
		}
	}

	m_CommandList->SetDescriptorHeaps(numDescriptor, desHeap);
}
