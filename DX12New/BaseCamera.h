#pragma once
#include "Vector.h"
#include "Matrix.h"


class BaseCamera
{
public:
	void Update();

	void SetEyeAtUp(Vector3 _eye, Vector3 _at, Vector3 _up);
	void SetLookDirection(Vector3 _forward, Vector3 _up);
	void SetRotation(Vector3 _rotate) { m_Rotation = m_Position; }
	void SetPosition(Vector3 _pos) { m_Position = _pos; }

	const Vector3 GetRotation() { return m_Rotation; }
	const Vector3 GetPosition() { return m_Position; }
	const Vector3 GetForward() { return m_Basis.GetZ(); }
	const Vector3 GetUp() { return m_Basis.GetY(); }
	const Vector3 GetRight() { return m_Basis.GetX(); }

	const Matrix4& GetViewMatrix() { return m_ViewMatrix; }
	const Matrix4& GetProjMatrix() { return m_ProjMatrix; }
	const Matrix4& GetViewProjMatrix() { return m_ViewProjMatrix; }
	const Matrix4& GetPrevoiusViewProjMatrix() { return m_PrevoiusViewProjMatrix; }
	const Matrix4& GetInverseViewMatrix() { return m_InverseViewMatrix; }
	const Matrix4& GetInverseProjMatrix() { return m_InverseProjMatrix; }

protected:
	BaseCamera() : m_Basis(Tag::ONE), m_Position(Vector3(Tag::ZERO)), m_Rotation(Vector3(Tag::ZERO)){}

	void SetProjMatrix(const Matrix4& _mat) { m_ProjMatrix = _mat; }


	//�洢�����ǰ���ң��ϣ���������
	Matrix3 m_Basis;

	//�۲����
	Matrix4 m_ViewMatrix;
	//ͶӰ����
	Matrix4 m_ProjMatrix;
	//�۲�ͶӰ����
	Matrix4 m_ViewProjMatrix;
	//��һ֡�Ĺ۲�ͶӰ����
	Matrix4 m_PrevoiusViewProjMatrix;
	//��۲����
	Matrix4 m_InverseViewMatrix;
	//��ͶӰ����
	Matrix4 m_InverseProjMatrix;

	Vector3 m_Rotation;
	Vector3 m_Position;
};

