#pragma once

#include "Buffer.h"
#include "ByteAddressBuffer.h"

class StructuredBuffer :public Buffer
{
public:
	virtual size_t GetNumElements() const { return m_NumElements; }
										  
	virtual size_t GetElementSize() const { return m_ElementSize; }

	std::shared_ptr<ByteAddressBuffer> GetCounterBuffer() const { return m_CounterBuffer; }

protected:
	StructuredBuffer(Device& _device, size_t _numElements, size_t _elementSize);
	StructuredBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numElements, size_t _elementSize);

	virtual ~StructuredBuffer() = default;
private:
	size_t m_NumElements;
	size_t m_ElementSize;

	std::shared_ptr<ByteAddressBuffer> m_CounterBuffer;
};

