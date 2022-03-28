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

	//Ϊ�����ڵ�ָ��һ�����ַ�������
	const std::string& GetName() const;
	void SetName(const std::string& _name);

	//��ȡ�ֲ��任(����ڸ��ڵ��λ��)
	DirectX::XMMATRIX GetLocalTransform() const;
	void SetLocalTransform(const DirectX::XMMATRIX& _localTransform);

	//�õ��ֲ��任����
	DirectX::XMMATRIX GetInverseLocalTransform() const;

	//��ȡ����任
	DirectX::XMMATRIX GetWorldTransform() const;

	//��ȡ����任����
	DirectX::XMMATRIX GetInverseWorldTransform() const;

	//����һ���ӽڵ㵽�����ڵ�
	//������ڵ㱻ɾ��,��û������Ľڵ�����,�����ӽڵ㽫�ᱻɾ��
	void AddChild(std::shared_ptr<SceneNode> childNode);
	void RemoveChild(std::shared_ptr<SceneNode> childNode);
	void SetParent(std::shared_ptr<SceneNode> parentNode);

	//����һ�����񵽳����ڵ�
	//���������������б��е�����
	size_t AddMesh(std::shared_ptr<Mesh> _mesh);
	void RemoveMesh(std::shared_ptr<Mesh> _mesh);

	//��ȡ�����ڵ��е�ģ��
	std::shared_ptr<Mesh> GetMesh(size_t _index = 0);

	//��ȡAABB��Χ��,������ģ�͵�AABB
	const DirectX::BoundingBox& GetAABB()const;

	//����һ���۲���
	void Accept(Visitor& _visitor);

protected:
	DirectX::XMMATRIX GetParentWorldTransform() const;

private:
	using NodePtr = std::shared_ptr<SceneNode>;
	using NodeList = std::vector<NodePtr>;
	using NodeNameMap = std::multimap<std::string, NodePtr>;
	using MeshList = std::vector<std::shared_ptr<Mesh>>;

	std::string m_Name;

	//ȷ������16λ����
	struct alignas(16) AlignedData
	{
		DirectX::XMMATRIX m_LocalTransform;
		DirectX::XMMATRIX m_InverseTransform;
	} *m_AlignedData;

	std::weak_ptr<SceneNode> m_ParentNode;
	NodeList m_Children;
	NodeNameMap m_ChildrenByName;
	MeshList m_Meshes;

	//�ϲ������AABB
	DirectX::BoundingBox m_AABB;
};


