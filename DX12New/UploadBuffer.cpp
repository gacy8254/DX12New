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

	//�����ǰû���ѷ�����ڴ�ҳ�����ڴ�ҳû���㹻�Ŀռ䣬�ͷ���һ���µ��ڴ�ҳ
	if (!m_CurrentPage || !m_CurrentPage->HasSpace(_sizeInBytes, _alignment))
	{
		m_CurrentPage = RequestPage();
	}

	//�ӵ�ǰ�ڴ�ҳ�з����ڴ�
	return m_CurrentPage->Allocate(_sizeInBytes, _alignment);
}

void UploadBuffer::Reset()
{
	//����ǰҳ���ÿ�
	m_CurrentPage = nullptr;
	//���ڴ�ҳ�ظ��Ƶ������ڴ�ҳ����
	m_AvaliablePages = m_PagePool;
	//�������ڴ�ҳ�ÿ�
	for (auto page : m_AvaliablePages)
	{
		page->Reste();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;

	//������ڿ��õ��ڴ�ҳ���ͽ�������ȥ�����ӿ����ڴ�ҳ�����Ƴ�
	if (!m_AvaliablePages.empty())
	{
		page = m_AvaliablePages.front();
		m_AvaliablePages.pop_front();
	}
	//���û�п��õ��ڴ�ҳ���ʹ���һ���µ��ڴ�ҳ�������ڴ�ҳ����
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

	//����һ���ϴ����е���Դ
	auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto o = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&p,
		D3D12_HEAP_FLAG_NONE,
		&o,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	//��ȡ��Դ�ĵ�ַ
	m_GPUPtr = m_Resource->GetGPUVirtualAddress();
	m_Resource->Map(0, nullptr, &m_CPUPtr);
}

UploadBuffer::Page::~Page()
{
	//ȡ����Դ��ӳ�䣬������ַ��Ϊ��
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
	//�ж��Ƿ����㹻�Ŀռ�
	if (!HasSpace(_sizeInBytes, _alignment))
	{
		throw std::bad_alloc();
	}

	//��������Ĵ�С��ƫ����
	size_t alignedSize = Math::AlignUp(_sizeInBytes, _alignment);
	m_Offset = Math::AlignUp(m_Offset, _alignment);

	//��ֵ
	Allocation alloc;
	alloc.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
	alloc.GPU = m_GPUPtr + m_Offset;

	//����ƫ����
	m_Offset += alignedSize;

	return alloc;
}

void UploadBuffer::Page::Reste()
{
	m_Offset = 0;
}
