#pragma once
#include "Transform.h"
enum class Space
{
	Local,
	World
};

class BaseCamera
{
public:
	BaseCamera();
	virtual ~BaseCamera();

	//���ó���
	void XM_CALLCONV SetLookAt(Vector4 _eye, Vector4 _target, Vector4 _up);

	//��ȡ�۲����
	Matrix4 GetViewMatrix() const;
	//��ȡ�۲�������
	Matrix4 GetInserseViewMatrix() const;

	void SetProjection(float _fovY, float _aspect, float _zNear, float _zFar);
	//��ȡ͸�Ӿ���
	Matrix4 GetProjMatrix() const;
	//��ȡ͸�Ӿ������
	Matrix4 GetInserseProjMatrix() const;

	void SetFov(float _fovY);

	float GetFov();

	//���������λ��������ռ�
	void XM_CALLCONV SetTranslation(Vector4 _translation);
	Vector4 GetTranslation() const;

	//�����������ת������ռ�
	void XM_CALLCONV SetRotation(Vector4 _translation);
	//��ȡ�������ת��Ԫ��
	Vector4 GetRotation() const;

	//����������Ľ���(����ռ�)
	void XM_CALLCONV SetFocalPoint(Vector4 _FocalPoint);
	//��ȡ����Ľ���(����ռ�)
	Vector4 GetFocalPoint() const;

	void XM_CALLCONV Translate(Vector4 _translation, Space _space = Space::Local);
	void Rotate(Vector4 _quaternion);
	void XM_CALLCONV MoveFocalPoint(Vector4 _focalPoint, Space _space = Space::Local);

protected:
	virtual void UpdateViewMatrix() const;
	virtual void UpdateInerseMatrix() const;
	virtual void UpdateProjMatrix() const;
	virtual void UpdateInverseProjMatrix() const;

	__declspec(align(16)) struct AlignedData
	{
		//�������������
		Vector4 m_Translation;
		//��ת����ռ�,����һ����Ԫ��
		Vector4 m_Rotation;
		//����Ľ���(����ռ�)
		Vector4 m_FocalPoint;

		Matrix4 m_ViewMatrix, m_InverseViewMatrix;
		Matrix4 m_ProjMatrix, m_InverseProjMatrix;
	};
	AlignedData* pData;

	float m_Fov = 45.0f;
	float m_Aspect = 1.0f;
	float m_ZNear = 0.1f;
	float m_ZFar = 1000.0f;


	//�洢�����ǰ���ң��ϣ���������
	Matrix3 m_Basis;

	//���Ϊ���ʾ������Ҫ����
	mutable bool m_ViewDirty = true, m_InverseViewDirty = true;
	mutable bool m_ProjDirty = true, m_InverseProjDirty = true;
};

