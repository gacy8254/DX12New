#include "Buffer.h"

Buffer::Buffer(const std::wstring& _name /*= L""*/)
	:Resource(_name)
{

}

Buffer::Buffer(const D3D12_RESOURCE_DESC& _resDesc, size_t _numElements, size_t _elementSzie, const std::wstring& _name /*= L""*/)
	:Resource(_resDesc, nullptr, _name)
{
	CreateViews(_numElements, _elementSzie);
}

void Buffer::CreateViews(size_t _numElements, size_t _elementSize)
{
	throw std::exception("函数没有被重载实现");
}
