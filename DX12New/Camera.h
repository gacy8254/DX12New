#pragma once
#include <DirectXMath.h>

enum class Space
{
	Local,
	World
};

class Camera
{
public:
	Camera();
	virtual ~Camera();

	//���ó���
	void XM_CALLCONV SetLookAt(DirectX::FXMVECTOR _eye, DirectX::FXMVECTOR _target, DirectX::FXMVECTOR _up);

	//��ȡ�۲����
	DirectX::XMMATRIX GetViewMatrix() const;
	//��ȡ�۲�������
	DirectX::XMMATRIX GetInserseViewMatrix() const;

	void SetProjection(float _fovY, float _aspect, float _zNear, float _zFar);
	//��ȡ͸�Ӿ���
	DirectX::XMMATRIX GetProjMatrix() const;
	//��ȡ͸�Ӿ������
	DirectX::XMMATRIX GetInserseProjMatrix() const;

	void SetFov(float _fovY);

	float GetFov();

	//���������λ��������ռ�
	void XM_CALLCONV SetTranslation(DirectX::FXMVECTOR _translation);
	DirectX::XMVECTOR GetTranslation() const;

	//�����������ת������ռ�
	void XM_CALLCONV SetRotation(DirectX::FXMVECTOR _translation);
	//��ȡ�������ת��Ԫ��
	DirectX::XMVECTOR GetRotation() const;

	void XM_CALLCONV Translate(DirectX::FXMVECTOR _translation, Space _space = Space::Local);
	void Rotate(DirectX::FXMVECTOR _quaternion);

protected:
	virtual void UpdateViewMatrix() const;
	virtual void UpdateInerseMatrix() const;
	virtual void UpdateProjMatrix() const;
	virtual void UpdateInverseProjMatrix() const;

	__declspec(align(16)) struct AlignedData
	{
		//�������������
		DirectX::XMVECTOR m_Translation;
		//��ת����ռ�,����һ����Ԫ��
		DirectX::XMVECTOR m_Rotation;

		DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
		DirectX::XMMATRIX m_ProjMatrix, m_InverseProjMatrix;
	};
	AlignedData* pData;

	float m_Fov = 45.0f;
	float m_Aspect = 1.0f;
	float m_ZNear = 0.1f;
	float m_ZFar = 1000.0f;

	//���Ϊ���ʾ������Ҫ����
	mutable bool m_ViewDirty = true, m_InverseViewDirty = true;
	mutable bool m_ProjDirty = true, m_InverseProjDirty = true;

};

