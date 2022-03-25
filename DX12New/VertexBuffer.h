#pragma once
#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	

	//获取VBV
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_VBV; }

	size_t GetNumVertices() const { return m_NumVertices; }

	size_t GetVertexStride() const { return m_VertexStride; }

protected:
	VertexBuffer(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, size_t _numVertices, size_t _vertexStride);
	VertexBuffer(Device& _device, size_t _numVertices, size_t _vertexStride);
	virtual ~VertexBuffer();

	void CreateVertexBufferViews();
private:
	//顶点数量
	size_t m_NumVertices;
	//顶点大小
	size_t m_VertexStride;

	D3D12_VERTEX_BUFFER_VIEW m_VBV;
};

