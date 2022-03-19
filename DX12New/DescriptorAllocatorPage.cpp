#include "DescriptorAllocatorPage.h"
#include "D3D12LibPCH.h"

#include "Application.h"

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap)
	:m_Type(_type), m_NumDescriptorsInHeap(_numDescriptorPerHeap)
{
	auto device = Application::Get().GetDevice();

	//堆的描述
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = m_Type;
	desc.NumDescriptors = m_NumDescriptorsInHeap;

	//创建堆
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));

	//变量赋值
	m_BaseDescriptor = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_Type);
	m_NumFreeHandles = m_NumDescriptorsInHeap;

	//添加一个空闲的块
	AddNewBlock(0, m_NumFreeHandles);
}

bool DescriptorAllocatorPage::HasSpace(uint32_t _numDescriptor) const
{
	//lower_bound方法查找列表中不小于指定值得第一个条目
	return m_FreeListBySzie.lower_bound(_numDescriptor) != m_FreeListBySzie.end();
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t _numDescriptor)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	//检查有足够的空闲描述符可以分配，如果没有返回一个默认值
	if (_numDescriptor > m_NumDescriptorsInHeap)
	{
		return DescriptorAllocation();
	}

	auto smallestBlockIt = m_FreeListBySzie.lower_bound(_numDescriptor);
	//检查有足够的空闲描述符可以分配，如果没有返回一个默认值
	if (smallestBlockIt == m_FreeListBySzie.end())
	{
		return DescriptorAllocation();
	}
	//块的大小
	auto blockSize = smallestBlockIt->first;
	//块的偏移量迭代器
	auto offsetIt = smallestBlockIt->second;
	//偏移量
	auto offset = offsetIt->first;

	//从可用列表中移除即将分配的块
	m_FreeListBySzie.erase(smallestBlockIt);
	m_FreeListByOffset.erase(offsetIt);

	//计算新的偏移量和大小
	auto newOffset = offset + _numDescriptor;
	auto newSize = blockSize - _numDescriptor;
	//如果分派后块的大小不为0，则将其重新添加会列表
	if (newSize > 0)
	{
		AddNewBlock(newOffset, newSize);
	}
	//可用描述符减去已分配的描述符数
	m_NumFreeHandles -= _numDescriptor;

	//返回一个描述符分配器
	return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize), _numDescriptor, m_DescriptorHandleIncrementSize, shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& _descriptorHandle, uint64_t _frameNumber)
{
	//计算偏移量
	auto offset = ComputerOffset(_descriptorHandle.GetDescriptorHandle());

	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	//将描述符加入已废弃的列表
	//等待GPU完成帧的渲染后才会返回空闲列表
	m_StaleDescriptors.emplace(offset, _descriptorHandle.GetNumHandles(), _frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t _frameNumber)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	while (!m_StaleDescriptors.empty() && m_StaleDescriptors.front().Frame <= _frameNumber)
	{
		auto& staleDescriptor = m_StaleDescriptors.front();

		auto offset = staleDescriptor.Offset;
		auto numDescriptor = staleDescriptor.Size;

		FreeBlock(offset, numDescriptor);

		m_StaleDescriptors.pop();
	}
}

uint32_t DescriptorAllocatorPage::ComputerOffset(D3D12_CPU_DESCRIPTOR_HANDLE _handle)
{
	//计算并返回描述符的偏移量
	return static_cast<uint32_t>(_handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t _offset, uint32_t _numDescriptor)
{
	//emplace函数用于将元素插入的容器中
	//
	auto offsetIt = m_FreeListByOffset.emplace(_offset, _numDescriptor);
	auto sizeIt = m_FreeListBySzie.emplace(_numDescriptor, offsetIt.first);
	
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t _offset, uint32_t _numDescriptor)
{
	//查找大于偏移量的第一个元素
	auto  nextBlockIt = m_FreeListByOffset.upper_bound(_offset);
	
	auto prevBlockIt = nextBlockIt;

	//判断是否是第一个元素，如果不是就将其指向前一个元素，否则将其设置为end
	if (prevBlockIt != m_FreeListByOffset.begin())
	{
		--prevBlockIt;
	}
	else
	{
		prevBlockIt = m_FreeListByOffset.end();
	}

	//可用描述符数加上将要释放的数量
	m_NumFreeHandles += _numDescriptor;

	//如果前一个元素存在，判断前一个元素的偏移量加上块的大小，是否正好等于当前块的偏移量，如果是就说明两个块是相连的
	if (prevBlockIt != m_FreeListByOffset.end() && _offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		//重新设置块的偏移量和大小
		_offset = prevBlockIt->first;
		_numDescriptor += prevBlockIt->second.Size;

		//将前一个元素暂时从列表中移除
		m_FreeListBySzie.erase(prevBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(prevBlockIt);
	}

	//如果后一个元素存在，判断当前偏移量加上块的大小是否等于后一个元素的偏移量，如果是就说明两个块是相连的
	if (nextBlockIt != m_FreeListByOffset.end()&& _offset + _numDescriptor == nextBlockIt->first)
	{
		//重新设置块的偏移量和大小
		_numDescriptor += nextBlockIt->second.Size;

		//将后一个元素暂时从列表中移除
		m_FreeListBySzie.erase(nextBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(nextBlockIt);
	}

	//将块加入列表
	AddNewBlock(_offset, _numDescriptor);
}
