#pragma once
#include "BasePSO.h"
#include "VertexType.h"
#include "Transform.h"
#include "Texture.h"
#include "CommandList.h"

class TAAPSO :
    public BasePSO
{
public:
	enum RootParameters
	{
		MainPassCB, //cbuffer cbPass : register	(b1)
		Textures,  // Texture2D InputTexture,			register (t0)
				   // Texture2D historyTexture,			register (t1)
				   // Texture2D VelocityTexture,		register (t2)
				   // Texture2D DepthTexture,			register (t3)

				   NumRootParameters
	};

	enum TAATexture
	{
		InputTexture,
		HistoryTexture,
		VelocityTexture,
		DepthTexture,
		NumTextures
	};

	TAAPSO(std::shared_ptr<Device> _device);
	virtual ~TAAPSO() = default;

	void SetTexture(std::vector<std::shared_ptr<Texture>>& _textures)
	{
		m_Textures = _textures;
		m_DirtyFlags |= DF_Material;
	}

	//应用到渲染管线上
	void Apply(CommandList& _commandList) override;

private:
	std::vector<std::shared_ptr<Texture>> m_Textures;

};

