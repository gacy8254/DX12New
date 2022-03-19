#include "DescriptorAllocatorPage.h"
#include "D3D12LibPCH.h"

#include "Application.h"

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap)
	:m_Type(_type), m_NumDescriptorsInHeap(_numDescriptorPerHeap)
{
	auto device = Application::Get().GetDevice();

	//�ѵ�����
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = m_Type;
	desc.NumDescriptors = m_NumDescriptorsInHeap;

	//������
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));

	//������ֵ
	m_BaseDescriptor = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_Type);
	m_NumFreeHandles = m_NumDescriptorsInHeap;

	//���һ�����еĿ�
	AddNewBlock(0, m_NumFreeHandles);
}

bool DescriptorAllocatorPage::HasSpace(uint32_t _numDescriptor) const
{
	//lower_bound���������б��в�С��ָ��ֵ�õ�һ����Ŀ
	return m_FreeListBySzie.lower_bound(_numDescriptor) != m_FreeListBySzie.end();
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t _numDescriptor)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	//������㹻�Ŀ������������Է��䣬���û�з���һ��Ĭ��ֵ
	if (_numDescriptor > m_NumDescriptorsInHeap)
	{
		return DescriptorAllocation();
	}

	auto smallestBlockIt = m_FreeListBySzie.lower_bound(_numDescriptor);
	//������㹻�Ŀ������������Է��䣬���û�з���һ��Ĭ��ֵ
	if (smallestBlockIt == m_FreeListBySzie.end())
	{
		return DescriptorAllocation();
	}
	//��Ĵ�С
	auto blockSize = smallestBlockIt->first;
	//���ƫ����������
	auto offsetIt = smallestBlockIt->second;
	//ƫ����
	auto offset = offsetIt->first;

	//�ӿ����б����Ƴ���������Ŀ�
	m_FreeListBySzie.erase(smallestBlockIt);
	m_FreeListByOffset.erase(offsetIt);

	//�����µ�ƫ�����ʹ�С
	auto newOffset = offset + _numDescriptor;
	auto newSize = blockSize - _numDescriptor;
	//������ɺ��Ĵ�С��Ϊ0������������ӻ��б�
	if (newSize > 0)
	{
		AddNewBlock(newOffset, newSize);
	}
	//������������ȥ�ѷ������������
	m_NumFreeHandles -= _numDescriptor;

	//����һ��������������
	return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize), _numDescriptor, m_DescriptorHandleIncrementSize, shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& _descriptorHandle, uint64_t _frameNumber)
{
	//����ƫ����
	auto offset = ComputerOffset(_descriptorHandle.GetDescriptorHandle());

	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	//�������������ѷ������б�
	//�ȴ�GPU���֡����Ⱦ��Ż᷵�ؿ����б�
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
	//���㲢������������ƫ����
	return static_cast<uint32_t>(_handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t _offset, uint32_t _numDescriptor)
{
	//emplace�������ڽ�Ԫ�ز����������
	//
	auto offsetIt = m_FreeListByOffset.emplace(_offset, _numDescriptor);
	auto sizeIt = m_FreeListBySzie.emplace(_numDescriptor, offsetIt.first);
	
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t _offset, uint32_t _numDescriptor)
{
	//���Ҵ���ƫ�����ĵ�һ��Ԫ��
	auto  nextBlockIt = m_FreeListByOffset.upper_bound(_offset);
	
	auto prevBlockIt = nextBlockIt;

	//�ж��Ƿ��ǵ�һ��Ԫ�أ�������Ǿͽ���ָ��ǰһ��Ԫ�أ�����������Ϊend
	if (prevBlockIt != m_FreeListByOffset.begin())
	{
		--prevBlockIt;
	}
	else
	{
		prevBlockIt = m_FreeListByOffset.end();
	}

	//���������������Ͻ�Ҫ�ͷŵ�����
	m_NumFreeHandles += _numDescriptor;

	//���ǰһ��Ԫ�ش��ڣ��ж�ǰһ��Ԫ�ص�ƫ�������Ͽ�Ĵ�С���Ƿ����õ��ڵ�ǰ���ƫ����������Ǿ�˵����������������
	if (prevBlockIt != m_FreeListByOffset.end() && _offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		//�������ÿ��ƫ�����ʹ�С
		_offset = prevBlockIt->first;
		_numDescriptor += prevBlockIt->second.Size;

		//��ǰһ��Ԫ����ʱ���б����Ƴ�
		m_FreeListBySzie.erase(prevBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(prevBlockIt);
	}

	//�����һ��Ԫ�ش��ڣ��жϵ�ǰƫ�������Ͽ�Ĵ�С�Ƿ���ں�һ��Ԫ�ص�ƫ����������Ǿ�˵����������������
	if (nextBlockIt != m_FreeListByOffset.end()&& _offset + _numDescriptor == nextBlockIt->first)
	{
		//�������ÿ��ƫ�����ʹ�С
		_numDescriptor += nextBlockIt->second.Size;

		//����һ��Ԫ����ʱ���б����Ƴ�
		m_FreeListBySzie.erase(nextBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(nextBlockIt);
	}

	//��������б�
	AddNewBlock(_offset, _numDescriptor);
}
