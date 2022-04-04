#pragma once

#include "Transform.h"

#include <map>
#include <memory>
#include <string>
#include <vector>



#include <DirectXCollision.h>



class Mesh;
class CommandList;
class Visitor;

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:
	explicit SceneNode(const Matrix4& _localTransform = Matrix4());
	virtual ~SceneNode();

	//为场景节点指定一个名字方便搜索
	const std::string& GetName() const;
	void SetName(const std::string& _name);

	//获取局部变换(相对于父节点的位置)
	Matrix4 GetLocalTransform() const;
	void SetLocalTransform(const Matrix4& _localTransform);

	//得到局部变换的逆
	Matrix4 GetInverseLocalTransform() const;

	//获取世界变换
	Matrix4 GetWorldTransform() const;

	//获取世界变换的逆
	Matrix4 GetInverseWorldTransform() const;

	Vector4 GetPosition() const;
	void SetPosition(Vector4 _pos);

	Vector4 GetRotation() const;
	void SetRotation(Vector4 _rotation);

	Vector4 GetScale() const;
	void SetScale(Vector4 _scale);

	//增加一个子节点到场景节点
	//如果父节点被删除,且没有另外的节点引用,所有子节点将会被删除
	void AddChild(std::shared_ptr<SceneNode> childNode);
	void RemoveChild(std::shared_ptr<SceneNode> childNode);
	void SetParent(std::shared_ptr<SceneNode> parentNode);

	//获取所有子节点

	//增加一个网格到场景节点
	//返回网格在网格列表中的索引
	size_t AddMesh(std::shared_ptr<Mesh> _mesh);
	void RemoveMesh(std::shared_ptr<Mesh> _mesh);

	//获取场景节点中的模型
	std::shared_ptr<Mesh> GetMesh(size_t _index = 0);


	//获取AABB包围盒,是所有模型的AABB
	const DirectX::BoundingBox& GetAABB()const;

	//接受一个观察者
	void Accept(Visitor& _visitor);

	size_t GetSize();

protected:
	Matrix4 GetParentWorldTransform() const;

private:
	using NodePtr = std::shared_ptr<SceneNode>;
	using NodeList = std::vector<NodePtr>;
	using NodeNameMap = std::multimap<std::string, NodePtr>;
	using MeshList = std::vector<std::shared_ptr<Mesh>>;

	void UpdateLocalTransform() const;

	std::string m_Name;

	//确保数据16位对齐
	struct alignas(16) AlignedData
	{
		Matrix4 m_LocalTransform;
		Matrix4 m_InverseTransform;
		Vector4 m_Translate;
		Vector4 m_Rotation;
		Vector4 m_Scale;
	} *m_AlignedData;

	std::weak_ptr<SceneNode> m_ParentNode;
	NodeList m_Children;
	NodeNameMap m_ChildrenByName;
	MeshList m_Meshes;

	bool m_DirtyData = true;

	//合并网格的AABB
	DirectX::BoundingBox m_AABB;
};


