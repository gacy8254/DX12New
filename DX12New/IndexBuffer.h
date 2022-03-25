#pragma once
#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
	

	void CreateIndexBufferViews();

	size_t GetNumIndicies() const { return m_NumIndices; }

	DXGI_FORMAT GetIndexFormat() const { return m_IndexFormat; }

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_IBV; }


protected:
	IndexBuffer(Device& _device, size_t _numIndicies, DXGI_FORMAT _format);
	IndexBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numIndicies, DXGI_FORMAT _format);
	virtual ~IndexBuffer() = default;

private:
	size_t m_NumIndices;
	DXGI_FORMAT m_IndexFormat;

	D3D12_INDEX_BUFFER_VIEW m_IBV;
};

