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
	//��ȡ����,�жϲ����뵱ǰPASS��͸���������Ƿ�һ��
	auto material = _mesh.GetMaterial();
	if (material->IsTransparent() == m_Transparent)
	{
		//���ò��ʵ���Ⱦ����
		m_LightingPSO.SetMaterial(material);
		m_LightingPSO.Apply(m_CommandList);

		//Draw
		_mesh.Draw(m_CommandList);
	}
}

void SceneVisitor::Visit(SceneNode& sceneNode)
{
	//�����������
	auto world = sceneNode.GetWorldTransform();
	m_LightingPSO.SetWorldMatrix(world);
}

void SceneVisitor::Visit(Scene& scene)
{
	//���ù۲�����ͶӰ����
	m_LightingPSO.SetViewMatrix(m_Camera.GetViewMatrix());
	m_LightingPSO.SetProjectionMatrix(m_Camera.GetProjMatrix());
}
