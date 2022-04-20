#include "BaseCamera.h"
#include "ShaderDefinition.h"
#include <complex>

BaseCamera::BaseCamera()
{
	pData = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
	pData->m_Translation = Vector4(Tag::ZERO);
	pData->m_Rotation = Vector4(Tag::ONE);
	pData->m_FocalPoint = Vector4(Tag::ZERO);
	m_JitterX = 0.0f;
	m_JitterY = 0.0f;
}

BaseCamera::~BaseCamera()
{
	_aligned_free(pData);
}

void XM_CALLCONV BaseCamera::SetLookAt(Vector4 _eye, Vector4 _target, Vector4 _up)
{
	pData->m_ViewMatrix = Transform::MatrixLookATLH(_eye, _target, _up);

	pData->m_FocalPoint = _eye;
	pData->m_Rotation = Transform::QuaternionRotationMatrix(pData->m_ViewMatrix);

	m_InverseViewDirty = true;
	m_ViewDirty = false;
}

Matrix4 BaseCamera::GetViewMatrix() const
{
	if (m_ViewDirty)
	{
		UpdateViewMatrix();
	}

	return pData->m_ViewMatrix;
}

Matrix4 BaseCamera::GetInserseViewMatrix() const
{
	if (m_ViewDirty || m_InverseViewDirty)
	{
		pData->m_InverseViewMatrix = Transform::InverseMatrix(nullptr, GetViewMatrix());
		m_InverseViewDirty = false;
	}

	return pData->m_InverseViewMatrix;
}

void BaseCamera::SetProjection(float _fovY, float _aspect, float _zNear, float _zFar, bool _isProjection)
{
	if (_isProjection)
	{
		m_Fov = _fovY;
		m_Aspect = _aspect;
	}
	else
	{
		m_Width = _fovY;
		m_Height = _aspect;
	}
	

#if USE_REVERSE_Z
	m_ZFar = _zNear;
	m_ZNear = _zFar;
	//m_ZFar = _zFar;
	//m_ZNear = _zNear;
#else
	m_ZFar = _zFar;
	m_ZNear = _zNear;
#endif
	
	m_IsProjection = _isProjection;
	m_ProjDirty = true;
	m_InverseProjDirty = true;
}

Matrix4 BaseCamera::GetProjMatrix() const
{
	if (m_ProjDirty || m_UnjitterProjDirty)
	{
		UpdateUnjitteredProjMatrix();
		UpdateProjMatrix();
	}
	return pData->m_ProjMatrix;
}

Matrix4 BaseCamera::GetInserseProjMatrix() const
{
	if (m_ProjDirty || m_InverseProjDirty)
	{
		UpdateInverseProjMatrix();
	}

	return pData->m_InverseProjMatrix;
}

void BaseCamera::SetJitter(double _jitterX, double _jitterY)
{
	m_JitterX = (float)_jitterX;
	m_JitterY = (float)_jitterY;

	m_UnjitterProjDirty = true;
}

Matrix4 BaseCamera::GetUnjitteredInverseProjMatrix() const
{
	if (m_UnjitterProjDirty || m_UnjitterInverseProjDirty)
	{
		UpdateUnjitteredInversedProjMatrix();
	}
	return pData->m_UnjitteredInverseProjMatrix;
}

Matrix4 BaseCamera::GetUnjitteredProjMatrix() const
{
	if (m_UnjitterProjDirty)
	{
		UpdateUnjitteredProjMatrix();
	}
	return pData->m_UnjitteredProjMatrix;
}

void BaseCamera::SetFov(float _fovY)
{
	if (m_Fov != _fovY)
	{
		m_Fov = _fovY;
		m_UnjitterProjDirty = true;
		m_UnjitterInverseProjDirty = true;
		m_ProjDirty = true;
		m_InverseProjDirty = true;
	}
}

float BaseCamera::GetFov()
{
	return m_Fov;
}

void XM_CALLCONV BaseCamera::SetTranslation(Vector4 _translation)
{
	pData->m_Translation = _translation;
	m_ViewDirty = true;
}

Vector4 BaseCamera::GetTranslation() const
{
	return pData->m_Translation;
}

void XM_CALLCONV BaseCamera::SetRotation(Vector4 _translation)
{
	pData->m_Rotation = _translation;
	m_ViewDirty = true;
}

Vector4 BaseCamera::GetRotation() const
{
	return pData->m_Rotation;
}

void XM_CALLCONV BaseCamera::SetFocalPoint(Vector4 _FocalPoint)
{
	pData->m_FocalPoint = _FocalPoint;
	m_ViewDirty = true;
}

Vector4 BaseCamera::GetFocalPoint() const
{
	return pData->m_FocalPoint;
}

void XM_CALLCONV BaseCamera::Translate(Vector4 _translation, Space _space /*= Space::Local*/)
{
	switch (_space)
	{
	case Space::Local:
		pData->m_Translation += Transform::Vector4Rotate(_translation, pData->m_Rotation);
		break;
	case Space::World:
		pData->m_Translation += _translation;
		break;
	}

	pData->m_Translation.SetW(1.0f);

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void BaseCamera::Rotate(Vector4 _quaternion)
{
	pData->m_Rotation = Transform::QuaternionMultiply(pData->m_Rotation, _quaternion);

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void XM_CALLCONV BaseCamera::MoveFocalPoint(Vector4 _focalPoint, Space _space /*= Space::Local*/)
{
	switch (_space)
	{
	case Space::Local:
		pData->m_FocalPoint += Transform::Vector4Rotate(_focalPoint, pData->m_Rotation);
		break;
	case Space::World:
		pData->m_FocalPoint += _focalPoint;
		break;
	}

	pData->m_Translation.SetW(1.0f);

	m_ViewDirty = true;
	m_InverseViewDirty = true;
}

void BaseCamera::UpdateViewMatrix() const
{
	pData->m_PreviousViewMatrix = pData->m_ViewMatrix;
	Matrix4 rotationMatrix = Transform::MatrixTranspose(Transform::MatrixRotationQuaternion(pData->m_Rotation));
	Matrix4 translateMatrix = Transform::MatrixTranslateFromVector(-(pData->m_Translation));
	Matrix4 focalMatrix = Transform::MatrixTranslateFromVector(-(pData->m_FocalPoint));

	//pData->m_ViewMatrix = rotationMatrix * translateMatrix;
	pData->m_ViewMatrix = focalMatrix * rotationMatrix * translateMatrix;

	m_InverseViewDirty = true;
	m_ViewDirty = false;
}

void BaseCamera::UpdateInverseViewMatrix() const
{
	if (m_ViewDirty)
	{
		UpdateViewMatrix();
	}

	pData->m_InverseViewMatrix = Transform::InverseMatrix(nullptr, pData->m_ViewMatrix);
	m_InverseViewDirty = false;
}

void BaseCamera::UpdateProjMatrix() const
{
	DirectX::XMMATRIX proj = pData->m_UnjitteredProjMatrix;
	proj.r[2].m128_f32[0] += (float)m_JitterX;//_31
	proj.r[2].m128_f32[1] += (float)m_JitterY;//_32

	pData->m_ProjMatrix = proj;

	m_ProjDirty = false;
	m_InverseProjDirty = true;
}

void BaseCamera::UpdateInverseProjMatrix() const
{
	pData->m_InverseProjMatrix = Transform::InverseMatrix(nullptr, pData->m_ProjMatrix);
	m_InverseProjDirty = false;
}

void BaseCamera::UpdateUnjitteredProjMatrix() const
{
	pData->m_PreviousUnjitteredProjMatrix = pData->m_UnjitteredProjMatrix;

	if (m_IsProjection)
	{
		pData->m_UnjitteredProjMatrix = Transform::MatrixPerspectiveForLH(DirectX::XMConvertToRadians(m_Fov), m_Aspect, m_ZNear, m_ZFar);
	}
	else
	{
		pData->m_UnjitteredProjMatrix = Transform::MatrixOrthogonalForLH(m_Width, m_Height, m_ZNear, m_ZFar);
	}
	

	m_ProjDirty = true;
	m_InverseProjDirty = true;
	m_UnjitterProjDirty = false;
	m_UnjitterInverseProjDirty = true;
}

void BaseCamera::UpdateUnjitteredInversedProjMatrix() const
{
	pData->m_UnjitteredInverseProjMatrix = Transform::InverseMatrix(nullptr, pData->m_UnjitteredProjMatrix);
	m_UnjitterInverseProjDirty = false;
}
