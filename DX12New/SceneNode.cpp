#include "SceneNode.h"
#include "Visitor.h"
#include "Mesh.h"

using namespace DirectX;

SceneNode::SceneNode(const Matrix4& _localTransform /*= Matrix4Identity()*/)
	:m_Name("SceneNode"),
	m_AABB({0, 0, 0}, {0, 0, 0})
{
	m_AlignedData = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
	m_AlignedData->m_LocalTransform = _localTransform;
	m_AlignedData->m_InverseTransform = Transform::InverseMatrix(nullptr, _localTransform);
}

SceneNode::~SceneNode()
{
	_aligned_free(m_AlignedData);
}

const std::string& SceneNode::GetName() const
{
	return m_Name;
}

void SceneNode::SetName(const std::string& _name)
{
	m_Name = _name;
}

Matrix4 SceneNode::GetLocalTransform() const
{
	return m_AlignedData->m_LocalTransform;
}

void SceneNode::SetLocalTransform(const Matrix4& _localTransform)
{
	m_AlignedData->m_LocalTransform = _localTransform;
	m_AlignedData->m_InverseTransform = Transform::InverseMatrix(nullptr, _localTransform);
}

Matrix4 SceneNode::GetInverseLocalTransform() const
{
	return m_AlignedData->m_InverseTransform;
}

Matrix4 SceneNode::GetWorldTransform() const
{
	return m_AlignedData->m_LocalTransform * GetParentWorldTransform();
}

Matrix4 SceneNode::GetInverseWorldTransform() const
{
	return Transform::InverseMatrix(nullptr, GetWorldTransform());
}

void SceneNode::AddChild(std::shared_ptr<SceneNode> childNode)
{
	if (childNode)
	{
		//判断子节点是否已经存在
		NodeList::iterator it = std::find(m_Children.begin(), m_Children.end(), childNode);
		if (it == m_Children.end())
		{
			//获取子节点的世界变换
			//设置子节点的父节点为自身
			//用子节点的世界变换乘自身世界变幻矩阵的逆矩阵,得到相对于自身的子节点的局部变换矩阵
			//设置子节点的局部变换矩阵
			//将子节点信息添加到容器中
			Matrix4 worldTransform = childNode->GetWorldTransform();
			childNode->m_ParentNode = shared_from_this();
			Matrix4 localTransform = worldTransform * GetInverseWorldTransform();
			childNode->SetLocalTransform(localTransform);
			m_Children.push_back(childNode);
			if (!childNode->GetName().empty())
			{
				m_ChildrenByName.emplace(childNode->GetName(), childNode);
			}
		}
	}
}

void SceneNode::RemoveChild(std::shared_ptr<SceneNode> childNode)
{
	if (childNode)
	{
		//判断子节点是否已经存在
		NodeList::iterator it = std::find(m_Children.begin(), m_Children.end(), childNode);
		if (it == m_Children.end())
		{
			//总子节点中移除
			childNode->SetParent(nullptr);
			m_Children.erase(it);

			NodeNameMap::iterator iter = m_ChildrenByName.find(childNode->GetName());
			if (iter != m_ChildrenByName.end())
			{
				m_ChildrenByName.erase(iter);
			}
		}
		else
		{
			//如果没有找到子节点
			//可能是子节点的子节点
			//遍历子节点尝试移除
			for (auto child : m_Children)
			{
				child->RemoveChild(childNode);
			}
		}
	}
}

void SceneNode::SetParent(std::shared_ptr<SceneNode> parentNode)
{
	std::shared_ptr<SceneNode>  me = shared_from_this();

	//如果传入的节点不会空就将其设为父节点
	if (parentNode)
	{
		parentNode->AddChild(me);
	}
	else if (auto parent = m_ParentNode.lock())
	{
		//传入一个空的节点
		//将父节点移除为空
		//设置局部变换为世界变换
		auto worldTransform = GetWorldTransform();
		parent->RemoveChild(me);
		m_ParentNode.reset();
		SetLocalTransform(worldTransform);
	}
}

size_t SceneNode::AddMesh(std::shared_ptr<Mesh> _mesh)
{
	size_t index = (size_t)-1;

	if (_mesh)
	{
		MeshList::iterator it = std::find(m_Meshes.begin(), m_Meshes.end(), _mesh);
		if (it == m_Meshes.cend())
		{
			index = m_Meshes.size();
			m_Meshes.push_back(_mesh);

			//合并包围盒
			BoundingBox::CreateMerged(m_AABB, m_AABB, _mesh->GetAABB());
		}
		else
		{
			index = it - m_Meshes.begin();
		}
	}

	return index;
}

void SceneNode::RemoveMesh(std::shared_ptr<Mesh> _mesh)
{
	if (_mesh)
	{
		MeshList::const_iterator iter = std::find(m_Meshes.begin(), m_Meshes.end(), _mesh);
		if (iter != m_Meshes.end())
		{
			m_Meshes.erase(iter);
		}
	}
}

std::shared_ptr<Mesh> SceneNode::GetMesh(size_t _index /*= 0*/)
{
	std::shared_ptr<Mesh> mesh = nullptr;

	if (_index < m_Meshes.size()) {
		mesh = m_Meshes[_index];
	}

	return mesh;
}

const DirectX::BoundingBox& SceneNode::GetAABB() const
{
	return m_AABB;
}

void SceneNode::Accept(Visitor& _visitor)
{
	_visitor.Visit(*this);

	// Visit meshes
	for (auto& mesh : m_Meshes)
	{
		mesh->Accept(_visitor);
	}

	// Visit children
	for (auto& child : m_Children)
	{
		child->Accept(_visitor);
	}
}

Matrix4 SceneNode::GetParentWorldTransform() const
{
	Matrix4 parentTransform = Matrix4();
	if (auto parentNode = m_ParentNode.lock())
	{
		parentTransform = parentNode->GetWorldTransform();
	}

	return parentTransform;
}
