#include "DescriptorAllocation.h"
#include "D3D12LibPCH.h"
#include "DescriptorAllocatorPage.h"

DescriptorAllocation::DescriptorAllocation()
	:m_Descriptor{0}, m_DescriptorSize(0), m_NumHandles(0), m_Page(nullptr)
{}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE _descriptor, uint32_t _numHandles, uint32_t _sescriptorSize, std::shared_ptr<DescriptorAllocatorPage> _page)
	: m_Descriptor(_descriptor), m_DescriptorSize(_sescriptorSize), m_NumHandles(_numHandles), m_Page(_page)
{}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
	: m_Descriptor(allocation.m_Descriptor), m_DescriptorSize(allocation.m_DescriptorSize), m_NumHandles(allocation.m_NumHandles), m_Page(allocation.m_Page)
{
	allocation.m_Descriptor.ptr = 0;
	allocation.m_NumHandles = 0;
	allocation.m_DescriptorSize = 0;
}

bool DescriptorAllocation::IsNull() const
{
	return m_Descriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t _offset /*= 0*/) const
{
	assert(_offset < m_NumHandles);
	//·µ»ØÆ«ÒÆÁ¿
	return { m_Descriptor.ptr + (m_DescriptorSize * _offset) };
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
	return m_NumHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage()
{
	return m_Page;
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && m_Page)
	{
		//Application::GetFrameCount();
		//m_Page->Free(std::move(*this), Application::GetFrameCount());

		m_Descriptor.ptr = 0;
		m_DescriptorSize = 0;
		m_NumHandles = 0;
		m_Page.reset();
	}
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other)
{
	Free();

	m_Descriptor = other.m_Descriptor;
	m_DescriptorSize = other.m_DescriptorSize;
	m_NumHandles = other.m_NumHandles;
	m_Page = std::move(other.m_Page);

	other.m_Descriptor.ptr = 0;
	other.m_NumHandles = 0;
	other.m_DescriptorSize = 0;

	return *this;
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}
