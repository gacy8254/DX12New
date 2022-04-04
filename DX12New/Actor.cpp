#include "Actor.h"

Actor::Actor()
{
	m_Mesh = std::make_shared<Mesh>();
	m_Material = std::make_shared<Material>();
}

void Actor::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveToplogy)
{
	m_Mesh->SetPrimitiveTopology(primitiveToplogy);

}

D3D12_PRIMITIVE_TOPOLOGY Actor::GetPrimitiveTopology() const
{
	return m_Mesh->GetPrimitiveTopology();
}

void Actor::SetVertexBuffer(uint32_t slotID, const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
	m_Mesh->SetVertexBuffer(slotID, vertexBuffer);
}

std::shared_ptr<VertexBuffer> Actor::GetVertexBuffer(uint32_t slotID) const
{
	return m_Mesh->GetVertexBuffer(slotID);
}

void Actor::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
	m_Mesh->SetIndexBuffer(indexBuffer);
}

std::shared_ptr<IndexBuffer> Actor::GetIndexBuffer()
{
	return m_Mesh->GetIndexBuffer();
}

size_t Actor::GetIndexCount() const
{
	return m_Mesh->GetIndexCount();
}

size_t Actor::GetVertexCount() const
{
	return m_Mesh->GetVertexCount();
}

void Actor::Draw(CommandList& commandList, uint32_t instanceCount /*= 1*/, uint32_t startInstance /*= 0*/)
{
	m_Mesh->Draw(commandList, instanceCount, startInstance);
}

void Actor::SetMaterial(std::shared_ptr<Material> material)
{
	m_Material = material;
}

std::shared_ptr<Material> Actor::GetMaterial() const
{
	return m_Material;
}

void Actor::SetAABB(const DirectX::BoundingBox& aabb)
{
	m_Mesh->SetAABB(aabb);
}

const DirectX::BoundingBox& Actor::GetAABB() const
{
	return m_Mesh->GetAABB();
}

void Actor::Accept(Visitor& visitor)
{
	visitor.Visit(*this);
}
