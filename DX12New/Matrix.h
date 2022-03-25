#pragma once

#include "Vector.h"

__declspec(align(16)) class Matrix3
{
public:
	inline Matrix3(Tag _tag);
	inline Matrix3() { m_Mat[0] = Vector3(Tag::ONE), m_Mat[1] = Vector3(Tag::ONE), m_Mat[2] = Vector3(Tag::ONE); }
	inline Matrix3(Vector3 _x, Vector3 _y, Vector3 _z) { m_Mat[0] = _x, m_Mat[1] = _y, m_Mat[2] = _z; }
	inline Matrix3(const Matrix3& _mat) { m_Mat[0] = _mat.m_Mat[0], m_Mat[1] = _mat.m_Mat[1], m_Mat[2] = _mat.m_Mat[2]; }
	inline Matrix3(const DirectX::XMMATRIX& _mat) { m_Mat[0] = Vector3(_mat.r[0]), m_Mat[1] = Vector3(_mat.r[1]), m_Mat[2] = Vector3(_mat.r[2]); }

	inline DirectX::XMMATRIX GetMatrix() { m_mat = DirectX::XMMATRIX(m_Mat[0], m_Mat[1], m_Mat[2], DirectX::XMVectorZero()); return m_mat; }

	inline void SetX(Vector3 _x) { m_Mat[0] = _x; }
	inline void SetY(Vector3 _y) { m_Mat[1] = _y; }
	inline void SetZ(Vector3 _z) { m_Mat[2] = _z; }

	inline Vector3 GetX() const { return m_Mat[0]; }
	inline Vector3 GetY() const { return m_Mat[1]; }
	inline Vector3 GetZ() const { return m_Mat[2]; }

	static inline Matrix3 MakeXRotation(float _angle) { return Matrix3(DirectX::XMMatrixRotationX(_angle)); }
	static inline Matrix3 MakeYRotation(float _angle) { return Matrix3(DirectX::XMMatrixRotationY(_angle)); }
	static inline Matrix3 MakeZRotation(float _angle) { return Matrix3(DirectX::XMMatrixRotationZ(_angle)); }
	static inline Matrix3 MakeScale(float _scale) { return Matrix3(DirectX::XMMatrixScaling(_scale, _scale, _scale)); }
	static inline Matrix3 MakeScale(float _x, float _y, float _z) { return Matrix3(DirectX::XMMatrixScaling(_x, _y, _z)); }
	static inline Matrix3 MakeScale(const DirectX::XMFLOAT3& _scale) { return Matrix3(DirectX::XMMatrixScaling(_scale.x, _scale.y, _scale.z)); }
	static inline Matrix3 MakeScale(Vector3 _scale) { return Matrix3(DirectX::XMMatrixScalingFromVector(_scale)); }

	inline operator DirectX::XMMATRIX() const { return DirectX::XMMATRIX(m_Mat[0], m_Mat[1], m_Mat[2], DirectX::XMVectorZero()); }

	inline Matrix3 operator* (float _v) const { return Matrix3(_v * GetX(), _v * GetY(), _v * GetZ()); }
	inline Vector3 operator* (Vector3 _vec) const { return Vector3(DirectX::XMVector3TransformNormal(_vec, *this)); }
	inline Matrix3 operator* (const Matrix3& _mat) const { return Matrix3(*this * _mat.GetX(), *this * _mat.GetY(), *this * _mat.GetZ()); }

protected:
private:
	Vector3 m_Mat[3];
	DirectX::XMMATRIX m_mat = DirectX::XMMatrixIdentity();
};

__declspec(align(16)) class Matrix4
{
public:
	inline Matrix4(Tag _tag);
	inline Matrix4() { m_Mat.r[0] = Vector4(Tag::ONE), m_Mat.r[1] = Vector4(Tag::ONE), m_Mat.r[2] = Vector3(Tag::ONE), m_Mat.r[3] = Vector3(Tag::ONE); }
	inline Matrix4(Vector3 _x, Vector3 _y, Vector3 _z, Vector3 _w) { m_Mat.r[0] = SetWToZero(_x), m_Mat.r[1] = SetWToZero(_y), m_Mat.r[2] = SetWToZero(_y), m_Mat.r[3] = SetWToOne(_w); }
	inline Matrix4(Vector4 _x, Vector4 _y, Vector4 _z, Vector4 _w) { m_Mat.r[0] = _x, m_Mat.r[1] = _y, m_Mat.r[2] = _z, m_Mat.r[3] = _w; }
	inline Matrix4(const float* _data) { m_Mat = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)_data); }
	inline Matrix4(const Matrix4& _mat) { m_Mat = _mat.m_Mat; }
	inline Matrix4(const Matrix3& _mat) 
	{ 
		m_Mat.r[0] = SetWToZero(_mat.GetX()), 
		m_Mat.r[1] = SetWToZero(_mat.GetY()), 
		m_Mat.r[2] = SetWToZero(_mat.GetZ()), 
		m_Mat.r[3] = CreateWUnitVector(); 
	}
	inline Matrix4(const Matrix3& _mat, Vector3 _w)
	{
		m_Mat.r[0] = SetWToZero(_mat.GetX()),
		m_Mat.r[1] = SetWToZero(_mat.GetY()),
		m_Mat.r[2] = SetWToZero(_mat.GetZ()),
		m_Mat.r[3] = SetWToOne(_w);
	}
	
	inline Vector4 GetX() const { return Vector4(m_Mat.r[0]); }
	inline Vector4 GetY() const { return Vector4(m_Mat.r[1]); }
	inline Vector4 GetZ() const { return Vector4(m_Mat.r[2]); }
	inline Vector4 GetW() const { return Vector4(m_Mat.r[3]); }

	inline void SetX(Vector4 x) { m_Mat.r[0] = x; }
	inline void SetY(Vector4 y) { m_Mat.r[1] = y; }
	inline void SetZ(Vector4 z) { m_Mat.r[2] = z; }
	inline void SetW(Vector4 w) { m_Mat.r[3] = w; }

	inline operator DirectX::XMMATRIX() const { return m_Mat; }

	inline Vector4 operator* (Vector3 vec) const { return Vector4(DirectX::XMVector3Transform(vec, m_Mat)); }
	inline Vector4 operator* (Vector4 vec) const { return Vector4(DirectX::XMVector4Transform(vec, m_Mat)); }
	inline Matrix4 operator* (const Matrix4& vec) const { return Matrix4(DirectX::XMMatrixMultiply(vec, m_Mat)); }

	inline Matrix4 RotateX(float _angle) { return Matrix4(DirectX::XMMatrixRotationX(_angle)); }
	inline Matrix4 RotateY(float _angle) { return Matrix4(DirectX::XMMatrixRotationY(_angle)); }
	inline Matrix4 RotateZ(float _angle) { return Matrix4(DirectX::XMMatrixRotationZ(_angle)); }
	inline Matrix4 RotateAxis(Vector3 _axis, float _angle) { return Matrix4(DirectX::XMMatrixRotationAxis(_axis, _angle)); }
	inline Matrix4 RotateQuaternion(Vector3 _quaternion) { return Matrix4(DirectX::XMMatrixRotationQuaternion(_quaternion)); }
	inline Matrix4 RotateRollPitchYaw(float _pitch, float _yaw, float _roll) { return Matrix4(DirectX::XMMatrixRotationRollPitchYaw(_pitch, _yaw, _roll)); }
	inline Matrix4 RotateRollPitchYawVector(Vector3 _angle) { return Matrix4(DirectX::XMMatrixRotationRollPitchYawFromVector(_angle)); }

	inline Matrix4 Scaling(float _scale) { return Matrix4(DirectX::XMMatrixScaling(_scale, _scale, _scale)); }
	inline Matrix4 Scaling(float _x, float _y, float _z) { return Matrix4(DirectX::XMMatrixScaling(_x, _y, _z)); }
	inline Matrix4 Scaling(Vector3 _scale) { return Matrix4(DirectX::XMMatrixScalingFromVector(_scale)); }

	inline Matrix4 Translation(float _x, float _y, float _z) { return Matrix4(DirectX::XMMatrixTranslation(_x, _y, _z)); }
	inline Matrix4 Translation(Vector3 _translate) { return Matrix4(DirectX::XMMatrixTranslationFromVector(_translate)); }

	inline Matrix4 Transpose(){ return Matrix4(DirectX::XMMatrixTranspose(m_Mat)); }

	inline Vector3 QuaternionFromMatrix() { return Vector3(DirectX::XMQuaternionRotationMatrix(m_Mat)); }

	static inline Matrix4 MakeRotateX(float _angle) { return Matrix4(DirectX::XMMatrixRotationX(_angle)); }
	static inline Matrix4 MakeRotateY(float _angle) { return Matrix4(DirectX::XMMatrixRotationY(_angle)); }
	static inline Matrix4 MakeRotateZ(float _angle) { return Matrix4(DirectX::XMMatrixRotationZ(_angle)); }
	static inline Matrix4 MakeRotateAxis(Vector3 _axis, float _angle) { return Matrix4(DirectX::XMMatrixRotationAxis(_axis, _angle)); }
	static inline Matrix4 MakeRotateQuaternion(Vector3 _quaternion) { return Matrix4(DirectX::XMMatrixRotationQuaternion(_quaternion)); }
	static inline Matrix4 MakeRotateRollPitchYaw(float _pitch, float _yaw, float _roll) { return Matrix4(DirectX::XMMatrixRotationRollPitchYaw(_pitch, _yaw, _roll)); }
	static inline Matrix4 MakeRotateRollPitchYawVector(Vector3 _angle) { return Matrix4(DirectX::XMMatrixRotationRollPitchYawFromVector(_angle)); }
			
	static inline Matrix4 MakeScaling(float _scale) { return Matrix4(DirectX::XMMatrixScaling(_scale, _scale, _scale)); }
	static inline Matrix4 MakeScaling(float _x, float _y, float _z) { return Matrix4(DirectX::XMMatrixScaling(_x, _y, _z)); }
	static inline Matrix4 MakeScaling(Vector3 _scale) { return Matrix4(DirectX::XMMatrixScalingFromVector(_scale)); }
				
	static inline Matrix4 MakeTranslation(float _x, float _y, float _z) { return Matrix4(DirectX::XMMatrixTranslation(_x, _y, _z)); }
	static inline Matrix4 MakeTranslation(Vector3 _translate) { return Matrix4(DirectX::XMMatrixTranslationFromVector(_translate)); }

protected:
private:
	DirectX::XMMATRIX m_Mat;
};