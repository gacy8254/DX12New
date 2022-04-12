#include "SceneVisitor.h"
#include "BaseCamera.h"
#include "CommandList.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "Actor.h"
#include "SceneNode.h"
#include "Window.h"
#include "RenderTarget.h"

#include <DirectXMath.h>

using namespace DirectX;

SceneVisitor::SceneVisitor(CommandList& _commandList, const BaseCamera& _camera, BasePSO& _pso, const std::shared_ptr<MainPass> _mainPassCB, bool _transparent)
	:m_CommandList(_commandList),
	m_Camera(_camera),
	m_LightingPSO(_pso),
	m_MainPassCB(_mainPassCB),
	m_Transparent(_transparent)
{
	m_ObjectCB = std::make_shared<ObjectCB>();
}

SceneVisitor::~SceneVisitor()
{
	//_aligned_free(m_ObjectCB);
	//_aligned_free(m_MainPassCB);
}

void SceneVisitor::Visit(Actor& _actor)
{
	//获取材质,判断材质与当前PASS的透明度设置是否一致
	auto material = _actor.GetMaterial();
	if (material->IsTransparent() == m_Transparent)
	{
		//设置材质到渲染管线
		m_LightingPSO.SetMaterial(material);
		m_LightingPSO.Apply(m_CommandList);

		//Draw
		
		if (_actor.GetIndexCount() == 6)
		{
			m_CommandList.Draw(4);
		}
		else
		{
			_actor.Draw(m_CommandList);
		}
	}
}

void SceneVisitor::Visit(SceneNode& sceneNode)
{
	//设置世界矩阵
	m_ObjectCB->PreviousWorld					= sceneNode.GetPreviousWorldMatrix();
	m_ObjectCB->World							= sceneNode.GetWorldTransform();
	m_ObjectCB->InverseTransposeWorld			= Transform::MatrixTranspose(Transform::InverseMatrix(nullptr, sceneNode.GetWorldTransform()));
	m_ObjectCB->TexcoordTransform				= sceneNode.GetTexcoordTransform();
	m_LightingPSO.SetObjectCB(m_ObjectCB);
}

void SceneVisitor::Visit(Scene& scene)
{
	m_MainPassCB->PreviousViewProj			= m_Camera.GetPreviousViewProjMatrix();
	m_MainPassCB->CameraPos					= m_Camera.GetFocalPoint();
	m_MainPassCB->NearZ						= m_Camera.GetNearZ();
	m_MainPassCB->FarZ						= m_Camera.GetFarZ();
	m_MainPassCB->Proj						= m_Camera.GetProjMatrix();
	m_MainPassCB->View						= m_Camera.GetViewMatrix();
	m_MainPassCB->InverseProj				= m_Camera.GetInserseProjMatrix();
	m_MainPassCB->InverseView				= m_Camera.GetInserseViewMatrix();
	m_MainPassCB->JitterX					= m_Camera.GetJitterX();
	m_MainPassCB->JitterY					= m_Camera.GetJitterY();
	m_MainPassCB->UnjitteredProj			= m_Camera.GetUnjitteredProjMatrix();
	m_MainPassCB->UnjitteredInverseProj		= m_Camera.GetUnjitteredInverseProjMatrix();
	m_MainPassCB->ViewProj					= m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();
	m_MainPassCB->InverseViewProj			= Transform::InverseMatrix(nullptr, m_MainPassCB->ViewProj);
	m_MainPassCB->UnjitteredViewProj		= m_Camera.GetViewMatrix() * m_Camera.GetUnjitteredProjMatrix();
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);
	m_MainPassCB->ViewProjTex = m_MainPassCB->ViewProj * T;

	m_LightingPSO.SetMainPassCB(m_MainPassCB);
}
