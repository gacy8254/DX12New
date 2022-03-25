#pragma once
//���ڴ�CPU���������з���������
//CPU�ɼ�������������CPU�ڴ����ݴ���Դ
//�����Դ���Ƶ�GPU�ڴ��й���ɫ��ʹ��
//���� RTV DSV CBV SRV UAV SAMPLER
//�ڼ�������Դ��ΪӦ�ó������������
#include "DescriptorAllocation.h"
#include "d3dx12.h"

#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

class DescriptorAllocatorPage;
class Device;

class DescriptorAllocator
{
public:
	//�Ӷ��з�������������������Ĭ�Ϸ���һ����Ҳ����ָ������
	DescriptorAllocation Allocate(uint32_t _numDescriptors = 1);

	//��һ֡�������Ѿ���Ч�������������ͷ�
	void ReleaseStaleDescriptor();

protected:
	friend class std::default_delete<DescriptorAllocator>;

	DescriptorAllocator(Device& _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap = 256);
	virtual ~DescriptorAllocator();
private:
	using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

	Device& m_Device;

	//�����µķ�����ҳ��
	std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

	//�ѵ�����
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	//����������������
	uint32_t m_NumDescriptorPerHeap;

	DescriptorHeapPool m_HeapPool;
	//�ѳ��п��öѵ����
	std::set<size_t> m_AvaliableHeaps;

	std::mutex m_AllocationMutex;
};

