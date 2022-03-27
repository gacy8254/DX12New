#pragma once
#include "Mesh.h"
#include "VertexType.h"
#include "CommandList.h"

#include <DirectXMath.h>
#include <memory>
#include "Scene.h"

class MeshHelper
{
public:
	using VertexCollection = std::vector<VertexPositionNormalTangentBitangentTexture>;
	using IndexCollection = std::vector<uint16_t>;

	//用于将模型几何图元左右手坐标系翻转的辅助函数
	static inline void ReverseWinding(IndexCollection& indices, VertexCollection& vertices);

	//翻转法线的辅助函数
	static inline void InvertNormals(VertexCollection& vertices);

	// Helper function to compute a point on a unit circle aligned to the x,z plane and centered at the origin.
	static inline DirectX::XMVECTOR GetCircleVector(size_t i, size_t tessellation) noexcept;

	//计算单位圆上切线的辅助函数
	static inline DirectX::XMVECTOR GetCircleTangent(size_t i, size_t tessellation) noexcept;

	//辅助生成一个封闭的圆锥或圆柱
	static void CreateCylinderCap(VertexCollection& vertices, IndexCollection& indices, size_t tessellation, float height,
		float radius, bool isTop);

    /**
    * Create a cube.
    *
    * @param size The size of one side of the cube.
    * @param reverseWinding Whether to reverse the winding order of the triangles (useful for skyboxes).
    */
    static std::shared_ptr<Scene> CreateCube(CommandList _commandlist, float size = 1.0, bool reverseWinding = false);

    /**
     * Create a sphere.
     *
     * @param radius Radius of the sphere.
     * @param tessellation Determines how smooth the sphere is.
     * @param reverseWinding Whether to reverse the winding order of the triangles (useful for sydomes).
     */
    static std::shared_ptr<Scene> CreateSphere(CommandList _commandlist, float radius = 0.5f, uint32_t tessellation = 16, bool reversWinding = false);

    /**
     * Create a Cylinder
     *
     * @param radius The radius of the primary axis of the cylinder.
     * @param hight The height of the cylinder.
     * @param tessellation How smooth the cylinder will be.
     * @param reverseWinding Whether to reverse the winding order of the triangles.
     */
    static std::shared_ptr<Scene> CreateCylinder(CommandList _commandlist, float radius = 0.5f, float height = 1.0f, uint32_t tessellation = 32,
        bool reverseWinding = false);

    /**
     * Create a cone.
     *
     * @param radius The radius of the base of the cone.
     * @param height The height of the cone.
     * @param tessellation How smooth to make the cone.
     * @param reverseWinding Whether to reverse the winding order of the triangles.
     */
    static std::shared_ptr<Scene> CreateCone(CommandList _commandlist, float radius = 0.5f, float height = 1.0f, uint32_t tessellation = 32,
        bool reverseWinding = false);

    /**
     * Create a torus.
     *
     * @param radius The radius of the torus.
     * @param thickness The The thickness of the torus.
     * @param tessellation The smoothness of the torus.
     * @param reverseWinding Reverse the winding order of the vertices.
     */
    static std::shared_ptr<Scene> CreateTorus(CommandList _commandlist, float radius = 0.5f, float thickness = 0.333f, uint32_t tessellation = 32,
        bool reverseWinding = false);

    /**
     * Create a plane.
     *
     * @param width The width of the plane.
     * @param height The height of the plane.
     * @reverseWinding Whether to reverse the winding order of the plane.
     */
    static std::shared_ptr<Scene> CreatePlane(CommandList _commandlist, float width = 1.0f, float height = 1.0f, bool reverseWinding = false);
};

