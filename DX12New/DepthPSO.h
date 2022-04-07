#pragma once
#include "BasePSO.h"
#include "VertexType.h"
#include "Transform.h"
#include "Texture.h"
#include "CommandList.h"

class DepthPSO :public BasePSO
{
public:
	//enum RootParameters
	//{
	//	Textures,  // Texture2D InputTexture,			register (t0)

	//			   NumRootParameters
	//};

	//enum TAATexture
	//{
	//	InputTexture,
	//	NumTextures
	//};

	DepthPSO(std::shared_ptr<Device> _device);
	virtual ~DepthPSO() = default;

	//void SetTexture(std::shared_ptr<Texture> _textures)
	//{
	//	m_Textures = _textures;
	//	m_DirtyFlags |= DF_Material;
	//}

	//应用到渲染管线上
	void Apply(CommandList & _commandList) override;

private:
	//std::shared_ptr<Texture> m_Textures;
};

