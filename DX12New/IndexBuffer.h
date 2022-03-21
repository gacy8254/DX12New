#pragma once
#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const std::wstring& _name = L"");
	virtual ~IndexBuffer();

	virtual void CreateViews(size_t _numElements, size_t _elementSize) override;

	size_t GetNumIndicies() const { return m_NumIndices; }

	DXGI_FORMAT GetIndexFormat() const { return m_IndexFormat; }

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_IBV; }

	/**
	* Get the SRV for a resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

	/**
	* Get the UAV for a (sub)resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

private:
	size_t m_NumIndices;
	DXGI_FORMAT m_IndexFormat;

	D3D12_INDEX_BUFFER_VIEW m_IBV;
};

