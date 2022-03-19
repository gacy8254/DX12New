#pragma once
#include <d3d12.h>

#include <cstdint>
#include <memory>

class DescriptorAllocatorPage;

class DescriptorAllocation
{
public:
	//����һ���յ�������
	DescriptorAllocation();
	//����һ����Ч��������
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE _descriptor, uint32_t _numHandles, uint32_t _sescriptorSize, std::shared_ptr<DescriptorAllocatorPage> _page);
	//������������������ҳ
	~DescriptorAllocation();

	//�����������
	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

	//�����ƶ�����
	DescriptorAllocation(DescriptorAllocation&& allocation);
	DescriptorAllocation& operator=(DescriptorAllocation&& other);

	//����Ƿ���һ����Ч��������
	bool IsNull() const;

	//��ȡָ��ƫ������������
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t _offset = 0);

	//��ȡ������������
	uint32_t GetNumHandles() const;

	//��ȡ�����Ǹ�������ҳ
	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage();

private:
	//�ͷ������������Ķ���
	void Free();

	//��һ���������ĵ�ַ
	D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;

	//������������
	uint32_t m_NumHandles;
	
	//�������Ĵ�С
	uint32_t m_DescriptorSize;

	//��Դ��������ҳ
	std::shared_ptr<DescriptorAllocatorPage> m_Page;
};

