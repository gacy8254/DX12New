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

	pData->m_Translation = _eye;
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

void BaseCamera::SetProjection(float _fovY, float _aspect, float _zNear, float _zFar)
{
	m_Fov = _fovY;
	m_Aspect = _aspect;

#if USE_REVERSE_Z
	m_ZFar = _zNear;
	m_ZNear = _zFar;
	//m_ZFar = _zFar;
	//m_ZNear = _zNear;
#else
	m_ZFar = _zFar;
	m_ZNear = _zNear;
#endif
	

	m_ProjDirty = true;
	m_InverseProjDirty = true;
}

Matrix4 BaseCamera::GetProjMatrix() const
{
	if (m_ProjDirty)
	{
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

void BaseCamera::SetUnjitteredProjMatrix(double _jitterX, double _jitterY)
{
	m_JitterX = (float)_jitterX;
	m_JitterY = (float)_jitterY;
}

Matrix4 BaseCamera::GetUnjitteredInverseProjMatrix() const
{
	if (m_ProjDirty)
	{
		UpdateProjMatrix();
	}
	UpdateUnjitteredProjMatrix();
	pData->m_UnjitteredInverseProjMatrix = Transform::InverseMatrix(nullptr, pData->m_UnjitteredProjMatrix);
	return pData->m_UnjitteredInverseProjMatrix;
}

Matrix4 BaseCamera::GetUnjitteredProjMatrix() const
{
	if (m_ProjDirty)
	{
		UpdateProjMatrix();
	}
	UpdateUnjitteredProjMatrix();
	return pData->m_UnjitteredProjMatrix;
}

void BaseCamera::SetFov(float _fovY)
{
	if (m_Fov != _fovY)
	{
		m_Fov = _fovY;
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
	Matrix4 rotationMatrix = Transform::MatrixTranspose(Transform::MatrixRotationQuaternion(pData->m_Rotation));
	Matrix4 translateMatrix = Transform::MatrixTranslateFromVector(-(pData->m_Translation));
	Matrix4 focalMatrix = Transform::MatrixTranslateFromVector(-(pData->m_FocalPoint));

	//pData->m_ViewMatrix = rotationMatrix * translateMatrix;
	pData->m_ViewMatrix = focalMatrix * rotationMatrix * translateMatrix;

	m_InverseViewDirty = true;
	m_ViewDirty = false;
}

void BaseCamera::UpdateInerseMatrix() const
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
	float fov = DirectX::XMConvertToRadians(m_Fov);
	pData->m_ProjMatrix = Transform::MatrixPerspectiveForLH(fov, m_Aspect, m_ZNear, m_ZFar);

	float xf = m_ZNear / (m_ZNear - m_ZFar);
	float x = std::tan(m_Fov * 0.5f);

	Vector4 r1 = Vector4(1 / (m_Aspect * x), 0, 0, 0);
	Vector4 r2 = Vector4(0, 1 / x, 0, 0);
	Vector4 r3 = Vector4(0, 0, xf, 1);
	Vector4 r4 = Vector4(0, 0, -m_ZFar * xf, 0);

	//Vector4 r1 = Vector4(0, 0, xf, 1);
	//Vector4 r2 = Vector4(x / m_Aspect, 0, 0, 0);
	//Vector4 r3 = Vector4(0, x, 0, 0);
	//Vector4 r4 = Vector4(0, 0, -m_ZFar * xf, 0);

	//pData->m_ProjMatrix = Matrix4(r1, r2, r3, r4);
	m_ProjDirty = false;
	m_InverseProjDirty = true;
	m_UnjitterProjDirty = true;
	m_UnjitterInverseProjDirty = true;
}

void BaseCamera::UpdateInverseProjMatrix() const
{
	pData->m_InverseProjMatrix = Transform::InverseMatrix(nullptr, pData->m_ProjMatrix);
	m_InverseProjDirty = false;
}

void BaseCamera::UpdateUnjitteredProjMatrix() const
{
	if (m_ProjDirty)
	{
		UpdateProjMatrix();
	}
	DirectX::XMMATRIX  temp = pData->m_ProjMatrix;

	//m_Jitter[0] = (float)_jitterX;
	//m_Jitter[1] = (float)_jitterY;
	//
	//temp.r[2].m128_f32[0] += (float)_jitterX;
	//temp.r[2].m128_f32[1] += (float)_jitterY;

	pData->m_UnjitteredProjMatrix = temp;

	m_UnjitterProjDirty = false;
}
