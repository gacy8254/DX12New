#include "Scene.h"
#include "D3D12LibPCH.h"
#include "CommandList.h"
#include "Device.h"
#include "Material.h"
#include "Mesh.h"
#include "SceneNode.h"
#include "Texture.h"
#include "VertexType.h"
#include "Visitor.h"
#include "Transform.h"


using namespace DirectX;

class ProgressHandler : public Assimp::ProgressHandler
{
public:
	ProgressHandler(const Scene& scene, const std::function<bool(float)> progressCallback)
		: m_Scene(scene)
		, m_ProgressCallback(progressCallback)
	{}

	virtual bool Update(float percentage) override
	{
		// Invoke the progress callback
		if (m_ProgressCallback)
		{
			return m_ProgressCallback(percentage);
		}

		return true;
	}

private:
	const Scene& m_Scene;
	std::function<bool(float)> m_ProgressCallback;
};

inline DirectX::BoundingBox CreateBoundingBox(const aiAABB& _aabb)
{
	XMVECTOR min = XMVectorSet(_aabb.mMin.x, _aabb.mMin.y, _aabb.mMin.z, 1.0f);
	XMVECTOR max = XMVectorSet(_aabb.mMax.x, _aabb.mMax.y, _aabb.mMax.z, 1.0f);

	BoundingBox bb;
	BoundingBox::CreateFromPoints(bb, min, max);

	return bb;
}

DirectX::BoundingBox Scene::GetAABB() const
{
	DirectX::BoundingBox aabb{ { 0, 0, 0 }, { 0, 0, 0 } };

	if (m_RootNode)
	{
		aabb = m_RootNode->GetAABB();
	}

	return aabb;
}

void Scene::Accept(Visitor& visitor)
{
	visitor.Visit(*this);
	if (m_RootNode)
	{
		m_RootNode->Accept(visitor);
	}
}

bool Scene::LoadSceneFromFile(CommandList& _commadnList, const std::wstring& _fileName, const std::function<bool(float)>& _loadingProgress)
{
	fs::path filePath = _fileName;
	fs::path exportPath = fs::path(filePath).replace_extension("assbin");

	//设置根路径
	fs::path parentPath;
	if (filePath.has_parent_path())
	{
		parentPath = filePath.parent_path();
	}
	else
	{
		parentPath = fs::current_path();
	}

	Assimp::Importer importer;
	const aiScene* scene;

	importer.SetProgressHandler(new ProgressHandler(*this, _loadingProgress));

	//检查是否有预处理的文件存在
	if (fs::exists(exportPath) && fs::is_regular_file(exportPath))
	{
		scene = importer.ReadFile(exportPath.string(), aiProcess_GenBoundingBoxes);
	}
	else
	{
		//没有预处理文件存在,记载并且处理文件
		importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
		importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

		unsigned int preprocessFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph | aiProcess_ConvertToLeftHanded | aiProcess_GenBoundingBoxes;

		scene = importer.ReadFile(filePath.string(), preprocessFlags);

		if (scene)
		{
			//导出预处理文件
			Assimp::Exporter exporter;
			exporter.Export(scene, "assbin", exportPath.string(), 0);
		}
	}

	if (!scene)
	{
		return false;
	}

	ImportScene(_commadnList, *scene, parentPath);

	return true;
}

bool Scene::LoadSceneFromString(CommandList& commandList, const std::string& _sceneStr, const std::string& _format)
{
	Assimp::Importer importer;
	const aiScene* scene = nullptr;

	importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

	unsigned int preprocessFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_GenBoundingBoxes;

	scene = importer.ReadFileFromMemory(_sceneStr.data(), _sceneStr.size(), preprocessFlags, _format.c_str());

	if (!scene)
	{
		return false;
	}

	ImportScene(commandList, *scene, fs::current_path());

	return true;
}

void Scene::ImportScene(CommandList& _commandList, const aiScene& _scene, std::filesystem::path _parentPath)
{
	if (m_RootNode)
	{
		m_RootNode.reset();
	}

	m_MaterilMap.clear();
	m_MaterialList.clear();
	m_Meshes.clear();

	for (unsigned int i = 0; i < _scene.mNumMaterials; ++i)
	{
		ImportMaterial(_commandList, *(_scene.mMaterials[i]), _parentPath);
	}

	for (unsigned int i = 0; i < _scene.mNumMeshes; ++i)
	{
		ImportMesh(_commandList, *(_scene.mMeshes[i]));
	}

	m_RootNode = ImportSceneNode(_commandList, nullptr, _scene.mRootNode);

}

void Scene::ImportMaterial(CommandList& _commandList, const aiMaterial& _material, std::filesystem::path _parentPath)
{
	aiString    materialName;		//材质名
	aiString    aiTexturePath;		//贴图路径
	aiTextureOp aiBlendOperation;	//混合模式
	float       blendFactor;		//混合系数
	aiColor4D   diffuseColor;		//漫反射颜色
	aiColor4D   specularColor;		//高光颜色
	aiColor4D   ambientColor;		//环境光颜色
	aiColor4D   emissiveColor;		//自发光颜色
	float       opacity;			//透明度
	float       indexOfRefraction;	//折射率
	float       reflectivity;		//反射率
	float       shininess;			//光滑度
	float       bumpIntensity;		//凹凸强度

	std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

	if (_material.Get(AI_MATKEY_COLOR_AMBIENT, ambientColor) == aiReturn_SUCCESS)
	{
		pMaterial->SetAmbientColor(Vector4(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a));
	}
	if (_material.Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == aiReturn_SUCCESS)
	{
		pMaterial->SetEmissiveColor(Vector4(emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveColor.a));
	}
	if (_material.Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS)
	{
		pMaterial->SetDiffuseColor(Vector4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a));
	}
	if (_material.Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == aiReturn_SUCCESS)
	{
		pMaterial->SetSpecularColor(Vector4(specularColor.r, specularColor.g, specularColor.b, specularColor.a));
	}
	if (_material.Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS)
	{
		pMaterial->SetSpecularPower(shininess);
	}
	if (_material.Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS)
	{
		pMaterial->SetOpacity(opacity);
	}
	if (_material.Get(AI_MATKEY_REFRACTI, indexOfRefraction))
	{
		pMaterial->SetIndexOfRefraction(indexOfRefraction);
	}
	if (_material.Get(AI_MATKEY_REFLECTIVITY, reflectivity) == aiReturn_SUCCESS)
	{
		pMaterial->SetReflectance(XMFLOAT4(reflectivity, reflectivity, reflectivity, reflectivity));
	}
	if (_material.Get(AI_MATKEY_BUMPSCALING, bumpIntensity) == aiReturn_SUCCESS)
	{
		pMaterial->SetBumpIntensity(bumpIntensity);
	}

	if (_material.GetTextureCount(aiTextureType_AMBIENT) > 0 && 
		_material.GetTexture(aiTextureType_AMBIENT, 0, &aiTexturePath, nullptr, nullptr, &blendFactor, &aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, true);
		pMaterial->SetTexture(Material::TextureType::AO, texture);
	}

	if (_material.GetTextureCount(aiTextureType_EMISSIVE) > 0 &&
		_material.GetTexture(aiTextureType_EMISSIVE, 0, &aiTexturePath, nullptr, nullptr, &blendFactor,
			&aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, true);
		pMaterial->SetTexture(Material::TextureType::Emissive, texture);
	}

	// Load diffuse textures.
	if (_material.GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
		_material.GetTexture(aiTextureType_DIFFUSE, 0, &aiTexturePath, nullptr, nullptr, &blendFactor,
			&aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, true);
		pMaterial->SetTexture(Material::TextureType::Diffuse, texture);
	}

	// Load specular texture.
	if (_material.GetTextureCount(aiTextureType_SPECULAR) > 0 &&
		_material.GetTexture(aiTextureType_SPECULAR, 0, &aiTexturePath, nullptr, nullptr, &blendFactor,
			&aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, true);
		pMaterial->SetTexture(Material::TextureType::Metaltic, texture);
	}

	// Load specular power texture.
	if (_material.GetTextureCount(aiTextureType_SHININESS) > 0 &&
		_material.GetTexture(aiTextureType_SHININESS, 0, &aiTexturePath, nullptr, nullptr, &blendFactor,
			&aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, false);
		pMaterial->SetTexture(Material::TextureType::Roughness, texture);
	}

	if (_material.GetTextureCount(aiTextureType_OPACITY) > 0 &&
		_material.GetTexture(aiTextureType_OPACITY, 0, &aiTexturePath, nullptr, nullptr, &blendFactor,
			&aiBlendOperation) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, false);
		pMaterial->SetTexture(Material::TextureType::Opacity, texture);
	}

	// Load normal map texture.
	if (_material.GetTextureCount(aiTextureType_NORMALS) > 0 &&
		_material.GetTexture(aiTextureType_NORMALS, 0, &aiTexturePath) == aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, false);
		pMaterial->SetTexture(Material::TextureType::Normal, texture);
	}
	// Load bump map (only if there is no normal map).
	else if (_material.GetTextureCount(aiTextureType_HEIGHT) > 0 &&
		_material.GetTexture(aiTextureType_HEIGHT, 0, &aiTexturePath, nullptr, nullptr, &blendFactor) ==
		aiReturn_SUCCESS)
	{
		fs::path texturePath(aiTexturePath.C_Str());
		auto     texture = _commandList.LoadTextureFromFile(_parentPath / texturePath, false);

		//有些材质实际上将法线贴图存储在凹凸贴图槽中
		//Assimp无法区分这两种纹理类型之间的差异
		//因此我们尝试根据其像素深度假设纹理是法线贴图还是凹凸贴图
		//凹凸贴图通常为8 BPP（灰度），法线贴图通常为24 BPP或更高
		Material::TextureType textureType =
			(texture->BitsPerPixel() >= 24) ? Material::TextureType::Normal : Material::TextureType::Bump;

		pMaterial->SetTexture(textureType, texture);
	}

	// m_MaterialMap.insert( MaterialMap::value_type( materialName.C_Str(), pMaterial ) );
	m_MaterialList.push_back(pMaterial);
}

void Scene::ImportMesh(CommandList& _commandList, const aiMesh& _mesh)
{
	auto mesh = std::make_shared<Mesh>();

	std::vector<VertexPositionNormalTangentBitangentTexture> vertexData(_mesh.mNumVertices);

	assert(_mesh.mMaterialIndex < m_MaterialList.size());
	mesh->SetMaterial(m_MaterialList[_mesh.mMaterialIndex]);

	unsigned int i;
	if (_mesh.HasPositions())
	{
		for (i = 0; i < _mesh.mNumVertices; ++i)
		{
			vertexData[i].Position = { _mesh.mVertices[i].x, _mesh.mVertices[i].y, _mesh.mVertices[i].z };
		}
	}

	if (_mesh.HasNormals())
	{
		for (i = 0; i < _mesh.mNumVertices; ++i)
		{
			vertexData[i].Normal = { _mesh.mNormals[i].x, _mesh.mNormals[i].y, _mesh.mNormals[i].z };
		}
	}

	if (_mesh.HasTangentsAndBitangents())
	{
		for (i = 0; i < _mesh.mNumVertices; ++i)
		{
			vertexData[i].Tangent = { _mesh.mTangents[i].x, _mesh.mTangents[i].y, _mesh.mTangents[i].z };
			vertexData[i].Bitangent = { _mesh.mBitangents[i].x, _mesh.mBitangents[i].y, _mesh.mBitangents[i].z };
		}
	}

	if (_mesh.HasTextureCoords(0))
	{
		for (i = 0; i < _mesh.mNumVertices; ++i)
		{
			vertexData[i].TexCoord = { _mesh.mTextureCoords[0][i].x, _mesh.mTextureCoords[0][i].y,
									   _mesh.mTextureCoords[0][i].z };
		}
	}

	auto vertexBuffer = _commandList.CopyVertexBuffer(vertexData);
	mesh->SetVertexBuffer(0, vertexBuffer);

	if (_mesh.HasFaces())
	{
		std::vector<unsigned int> indices;
		for (int i = 0; i < _mesh.mNumFaces; ++i)
		{
			const aiFace& face = _mesh.mFaces[i];
			if (face.mNumIndices == 3)
			{
				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}
		}

		if (indices.size() > 0)
		{
			std::shared_ptr<IndexBuffer> indexBuffer = _commandList.CopyIndexBuffer(indices);
			mesh->SetIndexBuffer(indexBuffer);
		}
	}

	mesh->SetAABB(CreateBoundingBox(_mesh.mAABB));

	m_Meshes.push_back(mesh);
}

std::shared_ptr<SceneNode> Scene::ImportSceneNode(CommandList& _commandList, std::shared_ptr<SceneNode> _parent, const aiNode* _aiNode)
{
	if (!_aiNode)
	{
		return nullptr;
	}

	std::shared_ptr<SceneNode> node = std::make_shared<SceneNode>(Matrix4(&(_aiNode->mTransformation.a1)));
	node->SetParent(_parent);

	if (_aiNode->mName.length > 0)
	{
		node->SetName(_aiNode->mName.C_Str());
	}

	for (unsigned int i = 0; i < _aiNode->mNumMeshes; ++i)
	{
		assert(_aiNode->mMeshes[i] < m_Meshes.size());

		std::shared_ptr<Mesh> pMesh = m_Meshes[_aiNode->mMeshes[i]];
		node->AddMesh(pMesh);
	}

	for (unsigned int i = 0; i < _aiNode->mNumChildren; ++i)
	{
		auto child = ImportSceneNode(_commandList, node, _aiNode->mChildren[i]);
		node->AddChild(child);
	}

	return node;
}
