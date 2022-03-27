#pragma once
#include "DescriptorAllocation.h"

#include <d3d12.h>
#include <wrl.h>
#include "d3dx12.h"

#include <map>
#include <memory>
#include <mutex>
#include <queue>

class Device;

class DescriptorAllocatorPage :public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
	

	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Type; }

	//检查是否有可用的空间
	bool HasSpace(uint32_t _numDescriptor) const;

	//检查有多少可用的空间
	uint32_t NumFreeHandles() const { return m_NumFreeHandles; }

	//分配描述符，如果分配失败分会一个空的描述符
	DescriptorAllocation Allocate(uint32_t _numDescriptor);

	//释放先前使用Allocate分配的DescriptorAllocation
	//不需要直接使用此方法，因为当DescriptorAllocation类不在使用时，会自动释放
	void Free(DescriptorAllocation&& _descriptorHandle, uint64_t _frameNumber);

	//将已经废弃的描述符重新加入堆中
	void ReleaseStaleDescriptors();

protected:
	DescriptorAllocatorPage(Device& _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap);
	virtual ~DescriptorAllocatorPage() = default;
	//计算到指定句柄的偏移量，用于在释放描述符时确定描述符的位置
	uint32_t ComputerOffset(D3D12_CPU_DESCRIPTOR_HANDLE _handle);

	//将描述符块加入空闲列表中，此方法用于初始化空闲列表（使用包含所有描述符的单个块），在分配期间拆分描述符块时，以及在描述符空闲时合并相邻块。
	void AddNewBlock(uint32_t _offset, uint32_t _numDescriptor);

	//释放描述符块，将废弃的描述符提交回描述符堆
	//检查是否可以合并空闲列表中的相邻块
	void FreeBlock(uint32_t _offset, uint32_t _numDescriptor);

private:
	using OffsetType = uint32_t;
	using SizeType = uint32_t;

	struct FreeBlockInfo;

	//偏移量，块的信息
	using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
	//块的大小，指向块的迭代器
	using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

	//空闲块的信息
	struct FreeBlockInfo
	{
		FreeBlockInfo(SizeType _size) :Size(_size){}

		//空闲块的大小
		SizeType Size;
		//块的迭代器
		FreeListBySize::iterator FreeListBySizeIt;
	};

	//过时的描述符
	struct StaleDescriptorInfo
	{
		StaleDescriptorInfo(OffsetType _offset, SizeType _size, uint64_t _frame) : Offset(_offset), Size(_size), Frame(_frame){}

		//堆中的偏移量
		OffsetType Offset;
		//描述符的数量
		SizeType Size;
		//被释放的帧
		uint64_t Frame;
	};
	using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

	//空闲列表根据偏移量排序
	FreeListByOffset m_FreeListByOffset;
	//空闲列表根据块的大小排序
	FreeListBySize m_FreeListBySzie;
	//废弃的描述符的队列
	StaleDescriptorQueue m_StaleDescriptors;

	//堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	//堆得类型
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	//堆的起始地址
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
	//描述符的大小
	uint32_t m_DescriptorHandleIncrementSize;
	//堆中描述符的数量
	uint32_t m_NumDescriptorsInHeap;
	//空闲的描述符数量
	uint32_t m_NumFreeHandles;

	std::mutex m_AllocationMutex;

	Device& m_Device;
};

