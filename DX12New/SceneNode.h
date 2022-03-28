#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <DirectXCollision.h>



class Mesh;
class CommandList;
class Visitor;

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:
	explicit SceneNode(const DirectX::XMMATRIX& _localTransform = DirectX::XMMatrixIdentity());
	virtual ~SceneNode();

	//为场景节点指定一个名字方便搜索
	const std::string& GetName() const;
	void SetName(const std::string& _name);

	//获取局部变换(相对于父节点的位置)
	DirectX::XMMATRIX GetLocalTransform() const;
	void SetLocalTransform(const DirectX::XMMATRIX& _localTransform);

	//得到局部变换的逆
	DirectX::XMMATRIX GetInverseLocalTransform() const;

	//获取世界变换
	DirectX::XMMATRIX GetWorldTransform() const;

	//获取世界变换的逆
	DirectX::XMMATRIX GetInverseWorldTransform() const;

	//增加一个子节点到场景节点
	//如果父节点被删除,且没有另外的节点引用,所有子节点将会被删除
	void AddChild(std::shared_ptr<SceneNode> childNode);
	void RemoveChild(std::shared_ptr<SceneNode> childNode);
	void SetParent(std::shared_ptr<SceneNode> parentNode);

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

protected:
	DirectX::XMMATRIX GetParentWorldTransform() const;

private:
	using NodePtr = std::shared_ptr<SceneNode>;
	using NodeList = std::vector<NodePtr>;
	using NodeNameMap = std::multimap<std::string, NodePtr>;
	using MeshList = std::vector<std::shared_ptr<Mesh>>;

	std::string m_Name;

	//确保数据16位对齐
	struct alignas(16) AlignedData
	{
		DirectX::XMMATRIX m_LocalTransform;
		DirectX::XMMATRIX m_InverseTransform;
	} *m_AlignedData;

	std::weak_ptr<SceneNode> m_ParentNode;
	NodeList m_Children;
	NodeNameMap m_ChildrenByName;
	MeshList m_Meshes;

	//合并网格的AABB
	DirectX::BoundingBox m_AABB;
};


