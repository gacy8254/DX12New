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

	//����Ƿ��п��õĿռ�
	bool HasSpace(uint32_t _numDescriptor) const;

	//����ж��ٿ��õĿռ�
	uint32_t NumFreeHandles() const { return m_NumFreeHandles; }

	//�������������������ʧ�ֻܷ�һ���յ�������
	DescriptorAllocation Allocate(uint32_t _numDescriptor);

	//�ͷ���ǰʹ��Allocate�����DescriptorAllocation
	//����Ҫֱ��ʹ�ô˷�������Ϊ��DescriptorAllocation�಻��ʹ��ʱ�����Զ��ͷ�
	void Free(DescriptorAllocation&& _descriptorHandle, uint64_t _frameNumber);

	//���Ѿ����������������¼������
	void ReleaseStaleDescriptors();

protected:
	DescriptorAllocatorPage(Device& _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, uint32_t _numDescriptorPerHeap);
	virtual ~DescriptorAllocatorPage() = default;
	//���㵽ָ�������ƫ�������������ͷ�������ʱȷ����������λ��
	uint32_t ComputerOffset(D3D12_CPU_DESCRIPTOR_HANDLE _handle);

	//�����������������б��У��˷������ڳ�ʼ�������б�ʹ�ð��������������ĵ����飩���ڷ����ڼ�����������ʱ���Լ�������������ʱ�ϲ����ڿ顣
	void AddNewBlock(uint32_t _offset, uint32_t _numDescriptor);

	//�ͷ��������飬���������������ύ����������
	//����Ƿ���Ժϲ������б��е����ڿ�
	void FreeBlock(uint32_t _offset, uint32_t _numDescriptor);

private:
	using OffsetType = uint32_t;
	using SizeType = uint32_t;

	struct FreeBlockInfo;

	//ƫ�����������Ϣ
	using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
	//��Ĵ�С��ָ���ĵ�����
	using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

	//���п����Ϣ
	struct FreeBlockInfo
	{
		FreeBlockInfo(SizeType _size) :Size(_size){}

		//���п�Ĵ�С
		SizeType Size;
		//��ĵ�����
		FreeListBySize::iterator FreeListBySizeIt;
	};

	//��ʱ��������
	struct StaleDescriptorInfo
	{
		StaleDescriptorInfo(OffsetType _offset, SizeType _size, uint64_t _frame) : Offset(_offset), Size(_size), Frame(_frame){}

		//���е�ƫ����
		OffsetType Offset;
		//������������
		SizeType Size;
		//���ͷŵ�֡
		uint64_t Frame;
	};
	using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

	//�����б����ƫ��������
	FreeListByOffset m_FreeListByOffset;
	//�����б���ݿ�Ĵ�С����
	FreeListBySize m_FreeListBySzie;
	//�������������Ķ���
	StaleDescriptorQueue m_StaleDescriptors;

	//��
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	//�ѵ�����
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	//�ѵ���ʼ��ַ
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
	//�������Ĵ�С
	uint32_t m_DescriptorHandleIncrementSize;
	//����������������
	uint32_t m_NumDescriptorsInHeap;
	//���е�����������
	uint32_t m_NumFreeHandles;

	std::mutex m_AllocationMutex;

	Device& m_Device;
};

