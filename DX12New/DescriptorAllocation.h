#pragma once
#include <d3d12.h>

#include <cstdint>
#include <memory>

class DescriptorAllocatorPage;

class DescriptorAllocation
{
public:
	//创建一个空的描述符
	DescriptorAllocation();
	//创建一个有效的描述符
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE _descriptor, uint32_t _numHandles, uint32_t _sescriptorSize, std::shared_ptr<DescriptorAllocatorPage> _page);
	//将描述符返回描述符页
	~DescriptorAllocation();

	//拷贝构造禁用
	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

	//允许移动构造
	DescriptorAllocation(DescriptorAllocation&& allocation);
	DescriptorAllocation& operator=(DescriptorAllocation&& other);

	//检查是否是一个有效的描述符
	bool IsNull() const;

	//获取指定偏移量的描述符
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t _offset = 0);

	//获取描述符的数量
	uint32_t GetNumHandles() const;

	//获取来自那个描述符页
	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage();

private:
	//释放描述符到来的堆中
	void Free();

	//第一个描述符的地址
	D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;

	//描述符的数量
	uint32_t m_NumHandles;
	
	//描述符的大小
	uint32_t m_DescriptorSize;

	//来源的描述符页
	std::shared_ptr<DescriptorAllocatorPage> m_Page;
};

