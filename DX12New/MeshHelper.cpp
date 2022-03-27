#include "MeshHelper.h"
using namespace DirectX;

void MeshHelper::ReverseWinding(IndexCollection& indices, VertexCollection& vertices)
{
	assert((indices.size() % 3) == 0);
	for (auto it = indices.begin(); it != indices.end(); it += 3)
	{
		std::swap(*it, *(it + 2));
	}

	for (auto it = vertices.begin(); it != vertices.end(); ++it)
	{
		it->TexCoord.x = (1.f - it->TexCoord.x);
	}
}

DirectX::XMVECTOR MeshHelper::GetCircleVector(size_t i, size_t tessellation) noexcept
{
	float angle = float(i) * DirectX::XM_2PI / float(tessellation);
	float dx, dz;

	DirectX::XMScalarSinCos(&dx, &dz, angle);

	DirectX::XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
	return v;
}

DirectX::XMVECTOR MeshHelper::GetCircleTangent(size_t i, size_t tessellation) noexcept
{
	float angle = (float(i) * DirectX::XM_2PI / float(tessellation)) + DirectX::XM_PIDIV2;
	float dx, dz;

	DirectX::XMScalarSinCos(&dx, &dz, angle);

	DirectX::XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
	return v;
}

void MeshHelper::CreateCylinderCap(VertexCollection& vertices, IndexCollection& indices, size_t tessellation, float height, float radius, bool isTop)
{
	// Create cap indices.
	for (size_t i = 0; i < tessellation - 2; i++)
	{
		size_t i1 = (i + 1) % tessellation;
		size_t i2 = (i + 2) % tessellation;

		if (isTop)
		{
			std::swap(i1, i2);
		}

		size_t vbase = vertices.size();
		indices.push_back(vbase + i2);
		indices.push_back(vbase + i1);
		indices.push_back(vbase);
	}

	// Which end of the cylinder is this?
	XMVECTOR normal = g_XMIdentityR1;
	XMVECTOR textureScale = g_XMNegativeOneHalf;

	if (!isTop)
	{
		normal = XMVectorNegate(normal);
		textureScale = XMVectorMultiply(textureScale, g_XMNegateX);
	}

	// Create cap vertices.
	for (size_t i = 0; i < tessellation; i++)
	{
		XMVECTOR circleVector = GetCircleVector(i, tessellation);
		XMVECTOR position = XMVectorAdd(XMVectorScale(circleVector, radius), XMVectorScale(normal, height));
		XMVECTOR textureCoordinate =
			XMVectorMultiplyAdd(XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, g_XMOneHalf);

		vertices.emplace_back(position, normal, textureCoordinate);
	}
}

std::shared_ptr<Scene> MeshHelper::CreateCube(CommandList _commandlist, float size /*= 1.0*/, bool reverseWinding /*= false*/)
{
	// Cube is centered at 0,0,0.
	float s = size * 0.5f;

	// 8 edges of cube.
	XMFLOAT3 p[8] = { { s, s, -s }, { s, s, s },   { s, -s, s },   { s, -s, -s },
					  { -s, s, s }, { -s, s, -s }, { -s, -s, -s }, { -s, -s, s } };
	// 6 face normals
	XMFLOAT3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
	// 4 unique texture coordinates
	XMFLOAT3 t[4] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 } };

	// Indices for the vertex positions.
	uint16_t i[24] = {
		0, 1, 2, 3,  // +X
		4, 5, 6, 7,  // -X
		4, 1, 0, 5,  // +Y
		2, 7, 6, 3,  // -Y
		1, 4, 7, 2,  // +Z
		5, 0, 3, 6   // -Z
	};

	VertexCollection vertices;
	IndexCollection  indices;

	for (uint16_t f = 0; f < 6; ++f)  // For each face of the cube.
	{
		// Four vertices per face.
		vertices.emplace_back(p[i[f * 4 + 0]], n[f], t[0]);
		vertices.emplace_back(p[i[f * 4 + 1]], n[f], t[1]);
		vertices.emplace_back(p[i[f * 4 + 2]], n[f], t[2]);
		vertices.emplace_back(p[i[f * 4 + 3]], n[f], t[3]);

		// First triangle.
		indices.emplace_back(f * 4 + 0);
		indices.emplace_back(f * 4 + 1);
		indices.emplace_back(f * 4 + 2);

		// Second triangle
		indices.emplace_back(f * 4 + 2);
		indices.emplace_back(f * 4 + 3);
		indices.emplace_back(f * 4 + 0);
	}

	if (reverseWinding)
	{
		ReverseWinding(indices, vertices);
	}

	return _commandlist.CreateScene(vertices, indices);
}

std::shared_ptr<Scene> MeshHelper::CreateSphere(CommandList _commandlist, float radius /*= 0.5f*/, uint32_t tessellation /*= 16*/, bool reversWinding /*= false*/)
{
	if (tessellation < 3)
		throw std::out_of_range("tessellation parameter out of range");

	VertexCollection vertices;
	IndexCollection  indices;

	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	// Create rings of vertices at progressively higher latitudes.
	for (size_t i = 0; i <= verticalSegments; i++)
	{
		float v = 1 - (float)i / verticalSegments;

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			float u = (float)j / horizontalSegments;

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			auto normal = XMVectorSet(dx, dy, dz, 0);
			auto textureCoordinate = XMVectorSet(u, v, 0, 0);
			auto position = normal * radius;

			vertices.emplace_back(position, normal, textureCoordinate);
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;

	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			indices.push_back(i * stride + nextJ);
			indices.push_back(nextI * stride + j);
			indices.push_back(i * stride + j);

			indices.push_back(nextI * stride + nextJ);
			indices.push_back(nextI * stride + j);
			indices.push_back(i * stride + nextJ);
		}
	}

	if (reversWinding)
	{
		ReverseWinding(indices, vertices);
	}

	return _commandlist.CreateScene(vertices, indices);
}

std::shared_ptr<Scene> MeshHelper::CreateCylinder(CommandList _commandlist, float radius /*= 0.5f*/, float height /*= 1.0f*/, uint32_t tessellation /*= 32*/, bool reverseWinding /*= false*/)
{
	if (tessellation < 3)
		throw std::out_of_range("tessellation parameter out of range");

	VertexCollection vertices;
	IndexCollection  indices;

	height /= 2;

	XMVECTOR topOffset = XMVectorScale(g_XMIdentityR1, height);

	size_t stride = tessellation + 1;

	// Create a ring of triangles around the outside of the cylinder.
	for (size_t i = 0; i <= tessellation; i++)
	{
		XMVECTOR normal = GetCircleVector(i, tessellation);

		XMVECTOR sideOffset = XMVectorScale(normal, radius);

		float u = float(i) / float(tessellation);

		XMVECTOR textureCoordinate = XMLoadFloat(&u);

		vertices.emplace_back(XMVectorAdd(sideOffset, topOffset), normal, textureCoordinate);
		vertices.emplace_back(XMVectorSubtract(sideOffset, topOffset), normal,
			XMVectorAdd(textureCoordinate, g_XMIdentityR1));

		indices.push_back(i * 2 + 1);
		indices.push_back((i * 2 + 2) % (stride * 2));
		indices.push_back(i * 2);

		indices.push_back((i * 2 + 3) % (stride * 2));
		indices.push_back((i * 2 + 2) % (stride * 2));
		indices.push_back(i * 2 + 1);
	}

	// Create flat triangle fan caps to seal the top and bottom.
	CreateCylinderCap(vertices, indices, tessellation, height, radius, true);
	CreateCylinderCap(vertices, indices, tessellation, height, radius, false);

	// Build RH above
	if (reverseWinding)
	{
		ReverseWinding(indices, vertices);
	}

	return _commandlist.CreateScene(vertices, indices);
}

std::shared_ptr<Scene> MeshHelper::CreateCone(CommandList _commandlist, float radius /*= 0.5f*/, float height /*= 1.0f*/, uint32_t tessellation /*= 32*/, bool reverseWinding /*= false*/)
{
	if (tessellation < 3)
		throw std::out_of_range("tessellation parameter out of range");

	VertexCollection vertices;
	IndexCollection  indices;

	height /= 2;

	XMVECTOR topOffset = XMVectorScale(g_XMIdentityR1, height);

	size_t stride = tessellation + 1;

	// Create a ring of triangles around the outside of the cone.
	for (size_t i = 0; i <= tessellation; i++)
	{
		XMVECTOR circlevec = GetCircleVector(i, tessellation);

		XMVECTOR sideOffset = XMVectorScale(circlevec, radius);

		float u = float(i) / float(tessellation);

		XMVECTOR textureCoordinate = XMLoadFloat(&u);

		XMVECTOR pt = XMVectorSubtract(sideOffset, topOffset);

		XMVECTOR normal = XMVector3Cross(GetCircleTangent(i, tessellation), XMVectorSubtract(topOffset, pt));
		normal = XMVector3Normalize(normal);

		// Duplicate the top vertex for distinct normals
		vertices.emplace_back(topOffset, normal, g_XMZero);
		vertices.emplace_back(pt, normal, XMVectorAdd(textureCoordinate, g_XMIdentityR1));

		indices.push_back((i * 2 + 1) % (stride * 2));
		indices.push_back((i * 2 + 3) % (stride * 2));
		indices.push_back(i * 2);
	}

	// Create flat triangle fan caps to seal the bottom.
	CreateCylinderCap(vertices, indices, tessellation, height, radius, false);

	// Build RH above
	if (reverseWinding)
	{
		ReverseWinding(indices, vertices);
	}

	return _commandlist.CreateScene(vertices, indices);
}

std::shared_ptr<Scene> MeshHelper::CreateTorus(CommandList _commandlist, float radius /*= 0.5f*/, float thickness /*= 0.333f*/, uint32_t tessellation /*= 32*/, bool reverseWinding /*= false*/)
{
	assert(tessellation > 3);

	VertexCollection verticies;
	IndexCollection  indices;

	size_t stride = tessellation + 1;

	// First we loop around the main ring of the torus.
	for (size_t i = 0; i <= tessellation; i++)
	{
		float u = (float)i / tessellation;

		float outerAngle = i * XM_2PI / tessellation - XM_PIDIV2;

		// Create a transform matrix that will align geometry to
		// slice perpendicularly though the current ring position.
		XMMATRIX transform = XMMatrixTranslation(radius, 0, 0) * XMMatrixRotationY(outerAngle);

		// Now we loop along the other axis, around the side of the tube.
		for (size_t j = 0; j <= tessellation; j++)
		{
			float v = 1 - (float)j / tessellation;

			float innerAngle = j * XM_2PI / tessellation + XM_PI;
			float dx, dy;

			XMScalarSinCos(&dy, &dx, innerAngle);

			// Create a vertex.
			auto normal = XMVectorSet(dx, dy, 0, 0);
			auto position = normal * thickness / 2;
			auto textureCoordinate = XMVectorSet(u, v, 0, 0);

			position = XMVector3Transform(position, transform);
			normal = XMVector3TransformNormal(normal, transform);

			verticies.emplace_back(position, normal, textureCoordinate);

			// And create indices for two triangles.
			size_t nextI = (i + 1) % stride;
			size_t nextJ = (j + 1) % stride;

			indices.push_back(nextI * stride + j);
			indices.push_back(i * stride + nextJ);
			indices.push_back(i * stride + j);

			indices.push_back(nextI * stride + j);
			indices.push_back(nextI * stride + nextJ);
			indices.push_back(i * stride + nextJ);
		}
	}

	if (reverseWinding)
	{
		ReverseWinding(indices, verticies);
	}

	return _commandlist.CreateScene(verticies, indices);
}

std::shared_ptr<Scene> MeshHelper::CreatePlane(CommandList _commandlist, float width /*= 1.0f*/, float height /*= 1.0f*/, bool reverseWinding /*= false*/)
{
	using Vertex = VertexPositionNormalTangentBitangentTexture;

	// clang-format off
	// Define a plane that is aligned with the X-Z plane and the normal is facing up in the Y-axis.
	VertexCollection vertices = {
		Vertex(XMFLOAT3(-0.5f * width, 0.0f, 0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)),  // 0
		Vertex(XMFLOAT3(0.5f * width, 0.0f, 0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)),   // 1
		Vertex(XMFLOAT3(0.5f * width, 0.0f, -0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f)),  // 2
		Vertex(XMFLOAT3(-0.5f * width, 0.0f, -0.5f * height), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f))  // 3
	};
	// clang-format on
	IndexCollection indices = { 1, 3, 0, 2, 3, 1 };

	if (reverseWinding)
	{
		ReverseWinding(indices, vertices);
	}

	return _commandlist.CreateScene(vertices, indices);
}

void MeshHelper::InvertNormals(VertexCollection& vertices)
{
	for (auto it = vertices.begin(); it != vertices.end(); ++it)
	{
		it->Normal.x = -it->Normal.x;
		it->Normal.y = -it->Normal.y;
		it->Normal.z = -it->Normal.z;
	}
}
