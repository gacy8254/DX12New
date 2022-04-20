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

	//����ͶӰ���������
	//@Param _isProjection��ʾ����������������͸�Ӿ���,Ĭ��Ϊ͸�Ӿ���
	//���Ϊ���������� _fovY������ʾ��ͼ��� _aspect������ʾ��ͼ�߶�
	void SetProjection(float _fovY, float _aspect, float _zNear, float _zFar, bool _isProjection = true);
	//��ȡ͸�Ӿ���
	Matrix4 GetProjMatrix() const;
	//��ȡ͸�Ӿ������
	Matrix4 GetInserseProjMatrix() const;

	//��ȡ͸�Ӿ������
	void SetJitter(double _jitterX, double _jitterY);
	Matrix4 GetUnjitteredInverseProjMatrix() const;
	Matrix4 GetUnjitteredProjMatrix() const;

	Matrix4 GetPreviousViewProjMatrix() const
	{
		return pData->m_PreviousViewMatrix * pData->m_PreviousUnjitteredProjMatrix;
	}

	void SetFov(float _fovY);

	float GetFov();

	float GetNearZ() const { return m_ZNear; }
	float GetFarZ() const { return m_ZFar; }

	float GetJitterX() const { return m_JitterX; }
	float GetJitterY() const { return m_JitterY; }

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
	virtual void UpdateInverseViewMatrix() const;
	virtual void UpdateProjMatrix() const;
	virtual void UpdateInverseProjMatrix() const;
	virtual void UpdateUnjitteredProjMatrix() const;
	virtual void UpdateUnjitteredInversedProjMatrix() const;

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
		Matrix4 m_UnjitteredProjMatrix, m_UnjitteredInverseProjMatrix;
		Matrix4 m_PreviousViewMatrix, m_PreviousUnjitteredProjMatrix;
	};
	AlignedData* pData;

	float m_Width;
	float m_Height;
	float m_Fov = 45.0f;
	float m_Aspect = 1.0f;
	float m_ZNear = 1.0f;
	float m_ZFar = 50000.0f;

	float m_JitterX;
	float m_JitterY;

	//�洢�����ǰ���ң��ϣ���������
	Matrix3 m_Basis;

	//���Ϊ���ʾ������Ҫ����
	mutable bool m_ViewDirty = true, m_InverseViewDirty = true;
	mutable bool m_ProjDirty = true, m_InverseProjDirty = true;
	mutable bool m_UnjitterProjDirty = true, m_UnjitterInverseProjDirty = true;

	bool m_IsProjection = true;

};

