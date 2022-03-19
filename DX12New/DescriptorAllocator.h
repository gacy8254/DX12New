#pragma once
//用于从CPU描述符堆中分配描述符
//CPU可见描述符用于在CPU内存中暂存资源
//最后将资源复制到GPU内存中供着色器使用
//包括 RTV DSV CBV SRV UAV SAMPLER
//在加载新资源是为应用程序分配描述符
#include "DescriptorAllocation.h"
#include "d3dx12.h"

#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

class DescriptorAllocatorPage;

class DescriptorAllocator
{
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap = 256);
	virtual ~DescriptorAllocator();

	//从堆中分配多个连续的描述符，默认分配一个，也可以指定数量
	DescriptorAllocation Allocate(uint32_t _numDescriptors = 1);

	//当一帧结束后，已经无效的描述符奖杯释放
	void ReleaseStaleDescriptor(uint64_t _FrameNumber);

private:
	using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

	//创建新的分配器页面
	std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

	//堆的类型
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	//堆中描述符的数量
	uint32_t m_NumDescriptorPerHeap;

	DescriptorHeapPool m_HeapPool;
	//堆池中可用堆的序号
	std::set<size_t> m_AvaliableHeaps;

	std::mutex m_AllocationMutex;
};

