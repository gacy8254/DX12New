#include "UploadBuffer.h"
#include"D3D12LibPCH.h"
#include "helpers.h"
#include "Device.h"
#include "d3dx12.h"

//#include <new>

UploadBuffer::UploadBuffer(Device& _device, size_t pageSize /*= _2MB*/)
	:m_PageSize(pageSize), m_Device(_device)
{
}

UploadBuffer::~UploadBuffer()
{
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t _sizeInBytes, size_t _alignment)
{
	if (_sizeInBytes > m_PageSize)
	{
		throw std::bad_alloc();
	}

	//如果当前没有已分配的内存页或者内存页没有足够的空间，就分配一个新的内存页
	if (!m_CurrentPage || !m_CurrentPage->HasSpace(_sizeInBytes, _alignment))
	{
		m_CurrentPage = RequestPage();
	}

	//从当前内存页中分配内存
	return m_CurrentPage->Allocate(_sizeInBytes, _alignment);
}

void UploadBuffer::Reset()
{
	//将当前页面置空
	m_CurrentPage = nullptr;
	//将内存页池复制到可用内存页池中
	m_AvaliablePages = m_PagePool;
	//将所有内存页置空
	for (auto page : m_AvaliablePages)
	{
		page->Reste();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;

	//如果存在可用的内存页，就将其分配出去，并从可用内存页池中移除
	if (!m_AvaliablePages.empty())
	{
		page = m_AvaliablePages.front();
		m_AvaliablePages.pop_front();
	}
	//如果没有可用的内存页，就创建一个新的内存页，加入内存页池中
	else
	{
		page = std::make_shared<Page>(m_Device, m_PageSize);
		m_PagePool.push_back(page);
	}

	return page;
}

UploadBuffer::Page::Page(Device& _device, size_t _sizeInBytes)
	:m_PageSize(_sizeInBytes), m_Offset(0), m_CPUPtr(nullptr), m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0)), m_Device(_device)
{
	auto device = m_Device.GetD3D12Device();

	//创建一个上传堆中的资源
	auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto o = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&p,
		D3D12_HEAP_FLAG_NONE,
		&o,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	//获取资源的地址
	m_GPUPtr = m_Resource->GetGPUVirtualAddress();
	m_Resource->Map(0, nullptr, &m_CPUPtr);
}

UploadBuffer::Page::~Page()
{
	//取消资源的映射，并将地址置为空
	m_Resource->Unmap(0, nullptr);
	m_CPUPtr = nullptr;
	m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t _sizeInBytes, size_t _alignment) const
{
	size_t alignedSize = Math::AlignUp(_sizeInBytes, _alignment);
	size_t alignedOffset = Math::AlignUp(m_Offset, _alignment);

	return alignedOffset + alignedSize <= m_PageSize;
	return true;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t _sizeInBytes, size_t _alignment)
{
	//判断是否有足够的空间
	if (!HasSpace(_sizeInBytes, _alignment))
	{
		throw std::bad_alloc();
	}

	//计算对齐后的大小和偏移量
	size_t alignedSize = Math::AlignUp(_sizeInBytes, _alignment);
	m_Offset = Math::AlignUp(m_Offset, _alignment);

	//赋值
	Allocation alloc;
	alloc.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
	alloc.GPU = m_GPUPtr + m_Offset;

	//更新偏移量
	m_Offset += alignedSize;

	return alloc;
}

void UploadBuffer::Page::Reste()
{
	m_Offset = 0;
}
