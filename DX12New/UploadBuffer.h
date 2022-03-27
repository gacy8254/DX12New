#pragma once
#include "Defines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class Device;

class UploadBuffer
{
public:
	//用于上传资源到GPU
	struct Allocation 
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	//参数pageSize是用于分配给内存页的大小
	explicit UploadBuffer(Device& _device, size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	size_t GetPageSize() const { return m_PageSize; }

	//在上传堆中分配内存
	//分配的内存不能超过内存页的大小
	//使用memcpy或者类似的函数复制
	//将缓冲区数据赋值到allocation中的CPU指针
	//参数分别是需要分配的内存大小（以字节为单位）， 对齐方式
	Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

	//重置已分配的内存
	//只有当所有分配都不在队列中执行时才可以进行
	void Reset();

private:
	struct Page
	{
		Page(Device& _device, size_t _sizeInBytes);
		~Page();

		//检查是否有足够的内存去分配，如果不够就退出当前内存页并创建新的内存页
		bool HasSpace(size_t _sizeInBytes, size_t _alignment) const;

		//实际分配内存
		//抛出 std::bad_alloc 当需要分配的内存超过限度
		Allocation Allocate(size_t _sizeInBytes, size_t _alignment);

		//重置，将偏移量重置为0
		void Reste();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;

		Device& m_Device;

		//基础指针,指向分配内存的起始地址
		void* m_CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

		//分配的内存大小
		size_t m_PageSize;
		//偏移量
		size_t m_Offset;
	};
	//指向内存页面的指针的容器
	using PagePool = std::deque<std::shared_ptr<Page>>;

	//请求分配一个可用的内存页，如果没有就创建一个新的并加入内存页池
	std::shared_ptr<Page> RequestPage();

	Device& m_Device;

	//所有的内存页
	PagePool m_PagePool;
	//可用的内存页
	PagePool m_AvaliablePages;
	//当前的内存页
	std::shared_ptr<Page> m_CurrentPage;

	size_t m_PageSize;
};

