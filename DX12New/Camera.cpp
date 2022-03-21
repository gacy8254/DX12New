#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	pData = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
	pData->m_Translation = XMVectorZero();
	pData->m_Rotation = XMQuaternionIdentity();
}

Camera::~Camera()
{
	_aligned_free(pData);
}

void XM_CALLCONV Camera::SetLookAt(DirectX::FXMVECTOR _eye, DirectX::FXMVECTOR _target, DirectX::FXMVECTOR _up)
{
	pData->m_ViewMatrix = XMMatrixLookAtLH(_eye, _target, _up);

	pData->m_Translation = _eye;
	pData->m_Rotation = XMQuaternionRotationMatrix(XMMatrixTranspose(pData->m_ViewMatrix));

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
	if (m_ViewDirty)
	{
		UpdateViewMatrix();
	}
	return pData->m_ViewMatrix;
}

DirectX::XMMATRIX Camera::GetInserseViewMatrix() const
{
	if (m_InverseViewDirty)
	{
		UpdateInerseMatrix();
	}
	return pData->m_InverseViewMatrix;
}

void Camera::SetProjection(float _fovY, float _aspect, float _zNear, float _zFar)
{
	m_Fov = _fovY;
	m_Aspect = _aspect;
	m_ZFar = _zFar;
	m_ZNear = _zNear;

	m_ProjDirty = true;
	m_InverseProjDirty = true;
}

DirectX::XMMATRIX Camera::GetProjMatrix() const
{
	if (m_ProjDirty)
	{
		UpdateProjMatrix();
	}
	return pData->m_ProjMatrix;
}

DirectX::XMMATRIX Camera::GetInserseProjMatrix() const
{
	if (m_InverseProjDirty)
	{
		UpdateInverseProjMatrix();
	}
	return pData->m_InverseProjMatrix;

}

void Camera::SetFov(float _fovY)
{
	if (m_Fov != _fovY)
	{
		m_Fov = _fovY;
		m_ProjDirty = true;
		m_InverseProjDirty = true;
	}
	
}

float Camera::GetFov()
{
	return m_Fov;
}

void XM_CALLCONV Camera::SetTranslation(DirectX::FXMVECTOR _translation)
{
	pData->m_Translation = _translation;
}

DirectX::XMVECTOR Camera::GetTranslation() const
{
	return pData->m_Translation;
}

void XM_CALLCONV Camera::SetRotation(DirectX::FXMVECTOR _translation)
{
	pData->m_Rotation = _translation;
}

DirectX::XMVECTOR Camera::GetRotation() const
{
	return pData->m_Rotation;
}

void XM_CALLCONV Camera::Translate(DirectX::FXMVECTOR _translation, Space _space /*= Space::Local*/)
{
	switch (_space)
	{
	case Space::Local:
		pData->m_Translation += XMVector3Rotate(_translation, pData->m_Rotation);
		break;
	case Space::World:
		pData->m_Translation += _translation;
		break;
	}

	pData->m_Translation = XMVectorSetW(pData->m_Translation, 1.0f);

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void Camera::Rotate(DirectX::FXMVECTOR _quaternion)
{
	pData->m_Rotation = XMQuaternionMultiply(pData->m_Rotation, _quaternion);

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void Camera::UpdateViewMatrix() const
{
	XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(pData->m_Rotation));
	XMMATRIX translateMatrix = XMMatrixTranslationFromVector(-(pData->m_Translation));

	pData->m_ViewMatrix = translateMatrix * rotationMatrix;

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void Camera::UpdateInerseMatrix() const
{
	if (m_ViewDirty)
	{
		UpdateViewMatrix();
	}

	pData->m_InverseViewMatrix = XMMatrixInverse(nullptr, pData->m_ViewMatrix);
	m_InverseViewDirty = false;
}

void Camera::UpdateProjMatrix() const
{
	pData->m_ProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_Fov), m_Aspect, m_ZNear, m_ZFar);

	m_ProjDirty = false;
	m_InverseProjDirty = true;
}

void Camera::UpdateInverseProjMatrix() const
{
	if (m_ProjDirty)
	{
		UpdateProjMatrix();
	}

	pData->m_InverseProjMatrix = XMMatrixInverse(nullptr, pData->m_ProjMatrix);
	m_InverseProjDirty = false;
}
