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

	//设置朝向
	void XM_CALLCONV SetLookAt(Vector4 _eye, Vector4 _target, Vector4 _up);

	//获取观察矩阵
	Matrix4 GetViewMatrix() const;
	//获取观察矩阵的逆
	Matrix4 GetInserseViewMatrix() const;

	void SetProjection(float _fovY, float _aspect, float _zNear, float _zFar);
	//获取透视矩阵
	Matrix4 GetProjMatrix() const;
	//获取透视矩阵的逆
	Matrix4 GetInserseProjMatrix() const;

	void SetFov(float _fovY);

	float GetFov();

	//设置相机的位置在世界空间
	void XM_CALLCONV SetTranslation(Vector4 _translation);
	Vector4 GetTranslation() const;

	//设置相机的旋转在世界空间
	void XM_CALLCONV SetRotation(Vector4 _translation);
	//获取相机的旋转四元数
	Vector4 GetRotation() const;

	//设置相相机的焦点(世界空间)
	void XM_CALLCONV SetFocalPoint(Vector4 _FocalPoint);
	//获取相机的焦点(世界空间)
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
		//相机的世界坐标
		Vector4 m_Translation;
		//旋转世界空间,这是一个四元数
		Vector4 m_Rotation;
		//相机的焦点(世界空间)
		Vector4 m_FocalPoint;

		Matrix4 m_ViewMatrix, m_InverseViewMatrix;
		Matrix4 m_ProjMatrix, m_InverseProjMatrix;
	};
	AlignedData* pData;

	float m_Fov = 45.0f;
	float m_Aspect = 1.0f;
	float m_ZNear = 0.1f;
	float m_ZFar = 1000.0f;


	//存储相机的前，右，上，三个向量
	Matrix3 m_Basis;

	//如果为真表示矩阵需要更新
	mutable bool m_ViewDirty = true, m_InverseViewDirty = true;
	mutable bool m_ProjDirty = true, m_InverseProjDirty = true;
};

