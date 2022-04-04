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

	//Ϊ�����ڵ�ָ��һ�����ַ�������
	const std::string& GetName() const;
	void SetName(const std::string& _name);

	//��ȡ�ֲ��任(����ڸ��ڵ��λ��)
	Matrix4 GetLocalTransform() const;
	void SetLocalTransform(const Matrix4& _localTransform);

	//�õ��ֲ��任����
	Matrix4 GetInverseLocalTransform() const;

	//��ȡ����任
	Matrix4 GetWorldTransform() const;

	//��ȡ����任����
	Matrix4 GetInverseWorldTransform() const;

	Vector4 GetPosition() const;
	void SetPosition(Vector4 _pos);

	Vector4 GetRotation() const;
	void SetRotation(Vector4 _rotation);

	Vector4 GetScale() const;
	void SetScale(Vector4 _scale);

	//����һ���ӽڵ㵽�����ڵ�
	//������ڵ㱻ɾ��,��û������Ľڵ�����,�����ӽڵ㽫�ᱻɾ��
	void AddChild(std::shared_ptr<SceneNode> childNode);
	void RemoveChild(std::shared_ptr<SceneNode> childNode);
	void SetParent(std::shared_ptr<SceneNode> parentNode);

	//��ȡ�����ӽڵ�

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

	//ȷ������16λ����
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

	//�ϲ������AABB
	DirectX::BoundingBox m_AABB;
};


