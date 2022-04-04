#pragma once
#include "Visitor.h"
#include "Mesh.h"
#include "Material.h"

#include <DirectXCollision.h>  // For BoundingBox
#include <DirectXMath.h>       // For XMFLOAT3, XMFLOAT2

#include <d3d12.h>  // For D3D12_INPUT_LAYOUT_DESC, D3D12_INPUT_ELEMENT_DESC

#include <map>     // For std::map
#include <memory>  // For std::shared_ptr

class CommandList;
class IndexBuffer;
class VertexBuffer;

class Actor
{
public:
    Actor();
    virtual~Actor() = default;
    using BufferMap = std::map<uint32_t, std::shared_ptr<VertexBuffer>>;

    void                     SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveToplogy);
    D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const;

    void                          SetVertexBuffer(uint32_t slotID, const std::shared_ptr<VertexBuffer>& vertexBuffer);
    std::shared_ptr<VertexBuffer> GetVertexBuffer(uint32_t slotID) const;
    const BufferMap& GetVertexBuffers() const
    {
        return m_Mesh->GetVertexBuffers();
    }

    void                         SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);
    std::shared_ptr<IndexBuffer> GetIndexBuffer();

    /**
     * Get the number if indicies in the index buffer.
     * If no index buffer is bound to the mesh, this function returns 0.
     */
    size_t GetIndexCount() const;

    /**
     * Get the number of verticies in the mesh.
     * If this mesh does not have a vertex buffer, the function returns 0.
     */
    size_t GetVertexCount() const;

	void Draw(CommandList& commandList, uint32_t instanceCount = 1, uint32_t startInstance = 0);

	void                      SetMaterial(std::shared_ptr<Material> material);
	std::shared_ptr<Material> GetMaterial() const;

	void                        SetAABB(const DirectX::BoundingBox& aabb);
	const DirectX::BoundingBox& GetAABB() const;

    /**
    * Accept a visitor.
    */
    void Accept(Visitor& visitor);

    std::shared_ptr<Mesh> GetMesh() { return m_Mesh; }
    void SetMesh(std::shared_ptr<Mesh> _mesh) { m_Mesh = _mesh; }
private:
    std::shared_ptr<Mesh> m_Mesh;
    std::shared_ptr<Material> m_Material;
};

