#include "DescriptorAllocator.h"
#include "D3D12LibPCH.h"
#include "DescriptorAllocatorPage.h"
#include "Device.h"

struct MakeAllocatorPage : public DescriptorAllocatorPage
{
public:
	MakeAllocatorPage(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
		: DescriptorAllocatorPage(device, type, numDescriptors)
	{}

	virtual ~MakeAllocatorPage() {}
};

DescriptorAllocator::DescriptorAllocator(Device& _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap /*= 256*/)
	:m_Type(_type), m_NumDescriptorPerHeap(_numDescriptorPerHeap), m_Device(_device)
{}

DescriptorAllocator::~DescriptorAllocator()
{}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t _numDescriptors /*= 1*/)
{
	//锁定互斥锁，确保当前线程对分配器具有独占访问权
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	DescriptorAllocation allocation;

	//迭代可用的描述符堆
	for (auto iter = m_AvaliableHeaps.begin(); iter != m_AvaliableHeaps.end(); iter++)
	{
		auto allocatorPage = m_HeapPool[*iter];

		allocation = allocatorPage->Allocate(_numDescriptors);
		//如果分配完之后，分配器页的可用描述符句柄为0，就将当前页的索引从可用描述符堆中移除
		if (allocatorPage->NumFreeHandles() == 0)
		{
			iter = m_AvaliableHeaps.erase(iter);
		}

		//如果分配了一个有效的描述符句柄，中断循环
		if (!allocation.IsNull())
		{
			break;
		}
	}
	
	//如果没有分配到有效的描述符，说明描述符的数量超过了当前的最大数量
	//创建一个满足要求的新分配器页
	if (allocation.IsNull())
	{
		m_NumDescriptorPerHeap = std::max(m_NumDescriptorPerHeap, _numDescriptors);
		auto newPage = CreateAllocatorPage();

		allocation = newPage->Allocate(_numDescriptors);
	}

	return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptor()
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	for (size_t i = 0; i < m_HeapPool.size(); i++)
	{
		auto page = m_HeapPool[i];

		page->ReleaseStaleDescriptors();

		//将可用的页面加入可用堆集合中
		if (page->NumFreeHandles() > 0)
		{
			m_AvaliableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	//创建一个新的页面
	auto newPage = std::make_shared<MakeAllocatorPage>(m_Device, m_Type, m_NumDescriptorPerHeap);

	//将页面加入池中
	m_HeapPool.emplace_back(newPage);
	//将索引加入可用堆中
	m_AvaliableHeaps.insert(m_HeapPool.size() - 1);

	return newPage;
}
