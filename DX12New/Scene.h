#pragma once
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <assimp/scene.h>
#include <DirectXCollision.h> // For DirectX::BoundingBox

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

class aiMaterial;
class aiMesh;
class aiNode;

class CommandList;
class Device;
class SceneNode;
class Actor;
class Material;
class Visitor;

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	void SetRootNode(std::shared_ptr<SceneNode> _node) { m_RootNode = _node; }

	std::shared_ptr<SceneNode> GetRootNode() 
	{ 
		if (!m_RootNode)
		{
			m_RootNode = std::make_shared<SceneNode>();
		}
		return m_RootNode; 
	}

	//��ȡ���������İ�Χ��
	DirectX::BoundingBox GetAABB() const;

	virtual void Accept(Visitor& visitor);

protected:
	friend class CommandList;

	//���س������ļ�����
	bool LoadSceneFromFile(CommandList& _commadnList, const std::wstring& _fileName, const std::function<bool(float)>& _loadingProgress);

	//���ַ����м��س���
	//�������Ա�Ԥ����Ϊһ���ֽ�����,���Դ��ֽ������м��س���
	bool LoadSceneFromString(CommandList& commandList, const std::string& _sceneStr, const std::string& _format);

private:
	void ImportScene(CommandList& _commandList, const aiScene& _scene, std::filesystem::path _parentPath);
	void ImportMaterial(CommandList& _commandList, const aiMaterial& _material, std::filesystem::path _parentPath);
	void ImportMesh(CommandList& _commandList, const aiMesh& _mesh);
	std::shared_ptr<SceneNode> ImportSceneNode(CommandList& _commandList, std::shared_ptr<SceneNode> _parent, const aiNode* _aiNode);

	using MaterialMap = std::map<std::string, std::shared_ptr<Material>>;
	using MaterialList = std::vector<std::shared_ptr<Material>>;
	using ActorList = std::vector<std::shared_ptr<Actor>>;

	MaterialMap m_MaterilMap;
	MaterialList m_MaterialList;
	ActorList m_Actors;

	std::shared_ptr<SceneNode> m_RootNode = nullptr;

	std::wstring m_SceneFile;
};

