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
	//������������ȷ����ǰ�̶߳Է��������ж�ռ����Ȩ
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	DescriptorAllocation allocation;

	//�������õ���������
	for (auto iter = m_AvaliableHeaps.begin(); iter != m_AvaliableHeaps.end(); iter++)
	{
		auto allocatorPage = m_HeapPool[*iter];

		allocation = allocatorPage->Allocate(_numDescriptors);
		//���������֮�󣬷�����ҳ�Ŀ������������Ϊ0���ͽ���ǰҳ�������ӿ��������������Ƴ�
		if (allocatorPage->NumFreeHandles() == 0)
		{
			iter = m_AvaliableHeaps.erase(iter);
		}

		//���������һ����Ч��������������ж�ѭ��
		if (!allocation.IsNull())
		{
			break;
		}
	}
	
	//���û�з��䵽��Ч����������˵�������������������˵�ǰ���������
	//����һ������Ҫ����·�����ҳ
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

		//�����õ�ҳ�������öѼ�����
		if (page->NumFreeHandles() > 0)
		{
			m_AvaliableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	//����һ���µ�ҳ��
	auto newPage = std::make_shared<MakeAllocatorPage>(m_Device, m_Type, m_NumDescriptorPerHeap);

	//��ҳ��������
	m_HeapPool.emplace_back(newPage);
	//������������ö���
	m_AvaliableHeaps.insert(m_HeapPool.size() - 1);

	return newPage;
}
