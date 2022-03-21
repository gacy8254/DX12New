#pragma once
#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	VertexBuffer(const std::wstring& _name = L"");
	virtual ~VertexBuffer();

	//��BUFFER���м̳�
	virtual void CreateViews(size_t _numElements, size_t _elementSize) override;

	//��ȡVBV
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_VBV; }

	size_t GetNumVertices() const { return m_NumVertices; }

	size_t GetVertexStride() const { return m_VertexStride; }

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc /* = nullptr */) const override;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc /* = nullptr */) const override;

private:
	//��������
	size_t m_NumVertices;
	//�����С
	size_t m_VertexStride;

	D3D12_VERTEX_BUFFER_VIEW m_VBV;
};

