#include "SceneVisitor.h"
#include "EffectPSO.h"
#include "BaseCamera.h"
#include "CommandList.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "Mesh.h"
#include "SceneNode.h"

#include <DirectXMath.h>

using namespace DirectX;

SceneVisitor::SceneVisitor(CommandList& _commandList, const BaseCamera& _camera, EffectPSO& _pso, bool _transparent)
	:m_CommandList(_commandList),
	m_Camera(_camera),
	m_LightingPSO(_pso),
	m_Transparent(_transparent)
{
}

void SceneVisitor::Visit(Mesh& _mesh)
{
	//获取材质,判断材质与当前PASS的透明度设置是否一致
	auto material = _mesh.GetMaterial();
	if (material->IsTransparent() == m_Transparent)
	{
		//设置材质到渲染管线
		m_LightingPSO.SetMaterial(material);
		m_LightingPSO.Apply(m_CommandList);

		//Draw
		_mesh.Draw(m_CommandList);
	}
}

void SceneVisitor::Visit(SceneNode& sceneNode)
{
	//设置世界矩阵
	auto world = sceneNode.GetWorldTransform();
	m_LightingPSO.SetWorldMatrix(world);
}

void SceneVisitor::Visit(Scene& scene)
{
	//设置观察矩阵和投影矩阵
	m_LightingPSO.SetViewMatrix(m_Camera.GetViewMatrix());
	m_LightingPSO.SetProjectionMatrix(m_Camera.GetProjMatrix());
}
