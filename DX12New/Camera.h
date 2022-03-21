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

	//设置朝向
	void XM_CALLCONV SetLookAt(DirectX::FXMVECTOR _eye, DirectX::FXMVECTOR _target, DirectX::FXMVECTOR _up);

	//获取观察矩阵
	DirectX::XMMATRIX GetViewMatrix() const;
	//获取观察矩阵的逆
	DirectX::XMMATRIX GetInserseViewMatrix() const;

	void SetProjection(float _fovY, float _aspect, float _zNear, float _zFar);
	//获取透视矩阵
	DirectX::XMMATRIX GetProjMatrix() const;
	//获取透视矩阵的逆
	DirectX::XMMATRIX GetInserseProjMatrix() const;

	void SetFov(float _fovY);

	float GetFov();

	//设置相机的位置在世界空间
	void XM_CALLCONV SetTranslation(DirectX::FXMVECTOR _translation);
	DirectX::XMVECTOR GetTranslation() const;

	//设置相机的旋转在世界空间
	void XM_CALLCONV SetRotation(DirectX::FXMVECTOR _translation);
	//获取相机的旋转四元数
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
		//相机的世界坐标
		DirectX::XMVECTOR m_Translation;
		//旋转世界空间,这是一个四元数
		DirectX::XMVECTOR m_Rotation;

		DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
		DirectX::XMMATRIX m_ProjMatrix, m_InverseProjMatrix;
	};
	AlignedData* pData;

	float m_Fov = 45.0f;
	float m_Aspect = 1.0f;
	float m_ZNear = 0.1f;
	float m_ZFar = 1000.0f;

	//如果为真表示矩阵需要更新
	mutable bool m_ViewDirty = true, m_InverseViewDirty = true;
	mutable bool m_ProjDirty = true, m_InverseProjDirty = true;

};

