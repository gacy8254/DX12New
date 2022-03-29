#pragma once
#include <DirectXMath.h>

enum Tag
{
	ZERO,
	ONE,
	X,
	Y,
	Z,
	W
};

inline DirectX::XMVECTOR SetWToZero(DirectX::FXMVECTOR _vec) { return DirectX::XMVectorAndInt(_vec, DirectX::g_XMMask3); }
inline DirectX::XMVECTOR SetWToOne(DirectX::FXMVECTOR _vec) { return DirectX::XMVectorSelect(DirectX::g_XMIdentityR3, _vec, DirectX::g_XMMask3); }
inline DirectX::XMVECTOR CreateXUnitVector() { return DirectX::g_XMIdentityR0; }
inline DirectX::XMVECTOR CreateYUnitVector() { return DirectX::g_XMIdentityR1; }
inline DirectX::XMVECTOR CreateZUnitVector() { return DirectX::g_XMIdentityR2; }
inline DirectX::XMVECTOR CreateWUnitVector() { return DirectX::g_XMIdentityR3; }
inline DirectX::XMVECTOR SplatZero() { return DirectX::XMVectorZero(); }

class Vector4;

class Vector3
{
public:
	Vector3(Tag _tag);
	inline Vector3() {m_Vec = DirectX::XMVectorSplatOne();}
	inline Vector3(float _x, float _y, float _z) { m_Vec = DirectX::XMVectorSet(_x, _y, _z, _z); }
	inline Vector3(const DirectX::XMFLOAT3& _v) { m_Vec = DirectX::XMLoadFloat3(&_v); }
	inline Vector3(const Vector3& _v) { m_Vec = _v.m_Vec; }
	inline Vector3(const float _v) { m_Vec = DirectX::XMVectorReplicate(_v); }
	inline explicit Vector3(Vector4 _v);
	inline explicit Vector3(DirectX::FXMVECTOR _v) { m_Vec = _v; }

	//inline DirectX::XMFLOAT3 GetFloat3() const { return DirectX::XMStoreFloat3(m_Vec); }
	inline float GetX() const { return DirectX::XMVectorGetX(m_Vec); }
	inline float GetY() const { return DirectX::XMVectorGetY(m_Vec); }
	inline float GetZ() const { return DirectX::XMVectorGetZ(m_Vec); }
	inline void SetX(float _x) { m_Vec = DirectX::XMVectorSetX(m_Vec, _x); }
	inline void SetY(float _y) { m_Vec = DirectX::XMVectorSetY(m_Vec, _y); }
	inline void SetZ(float _z) { m_Vec = DirectX::XMVectorSetZ(m_Vec, _z); }

	//返回向量的反向
	inline Vector3 operator- () const { return Vector3(DirectX::XMVectorNegate(m_Vec)); }
	inline Vector3 operator+ (Vector3 _v2) const { return Vector3(DirectX::XMVectorAdd(m_Vec, _v2.m_Vec)); }
	inline Vector3 operator- (Vector3 _v2) const { return Vector3(DirectX::XMVectorSubtract(m_Vec, _v2.m_Vec)); }
	inline Vector3 operator* (Vector3 _v2) const { return Vector3(DirectX::XMVectorMultiply(m_Vec, _v2.m_Vec)); }
	inline Vector3 operator/ (Vector3 _v2) const { return Vector3(DirectX::XMVectorDivide(m_Vec, _v2.m_Vec)); }
	inline Vector3 operator+ (float _v2) const { return *this + Vector3(_v2); }
	inline Vector3 operator- (float _v2) const { return *this - Vector3(_v2); }
	inline Vector3 operator* (float _v2) const { return *this * Vector3(_v2); }
	inline Vector3 operator/ (float _v2) const { return *this / Vector3(_v2); }

	inline Vector3& operator += (Vector3 _v) { *this = *this + _v; return *this; }
	inline Vector3& operator -= (Vector3 _v) { *this = *this - _v; return *this; }
	inline Vector3& operator *= (Vector3 _v) { *this = *this * _v; return *this; }
	inline Vector3& operator /= (Vector3 _v) { *this = *this / _v; return *this; }

	inline operator DirectX::XMVECTOR() const { return m_Vec; }

	inline friend Vector3 operator+ (float _v1, Vector3 _v2) { return Vector3(_v1) + _v2; }
	inline friend Vector3 operator- (float _v1, Vector3 _v2) { return Vector3(_v1) - _v2; }
	inline friend Vector3 operator* (float _v1, Vector3 _v2) { return Vector3(_v1) * _v2; }
	inline friend Vector3 operator/ (float _v1, Vector3 _v2) { return Vector3(_v1) / _v2; }

	inline Vector3 LenghtSquare(Vector3 _vec) { return Vector3(DirectX::XMVector3LengthSq(_vec)); }

private:
	DirectX::XMVECTOR m_Vec;
};

class Vector4
{
public:
	Vector4(Tag _tag);
	inline Vector4() { m_Vec = DirectX::XMVectorSplatOne(); }
	inline Vector4(float _x, float _y, float _z, float _w) { m_Vec = DirectX::XMVectorSet(_x, _y, _z, _w); }
	inline Vector4(const DirectX::XMFLOAT4& _v) { m_Vec = DirectX::XMLoadFloat4(&_v); }
	inline Vector4(const Vector4& _v) { m_Vec = _v.m_Vec; }
	inline Vector4(const float _v) { m_Vec = DirectX::XMVectorReplicate(_v); }
	inline explicit Vector4(Vector3 _v) { m_Vec = DirectX::XMVectorSelect(DirectX::g_XMIdentityR3, _v, DirectX::g_XMMask3); }
	inline explicit Vector4(DirectX::FXMVECTOR _v) { m_Vec = _v; }

	inline DirectX::XMFLOAT4 GetFloat4() 
	{
		DirectX::XMStoreFloat4(&m_VecFloat4, m_Vec);
		return m_VecFloat4;
	}
	inline DirectX::XMVECTOR* GetPoint() { return &m_Vec; }
	inline float GetX() const { return DirectX::XMVectorGetX(m_Vec); }
	inline float GetY() const { return DirectX::XMVectorGetY(m_Vec); }
	inline float GetZ() const { return DirectX::XMVectorGetZ(m_Vec); }
	inline float GetW() const { return DirectX::XMVectorGetW(m_Vec); }
	inline void SetX(float _x) { m_Vec = DirectX::XMVectorSetX(m_Vec, _x); }
	inline void SetY(float _y) { m_Vec = DirectX::XMVectorSetY(m_Vec, _y); }
	inline void SetZ(float _z) { m_Vec = DirectX::XMVectorSetZ(m_Vec, _z); }
	inline void SetW(float _w) { m_Vec = DirectX::XMVectorSetW(m_Vec, _w); }

	inline operator DirectX::XMVECTOR() const { return m_Vec; }

	//返回向量的反向
	inline Vector4 operator- () const { return Vector4(DirectX::XMVectorNegate(m_Vec)); }
	inline Vector4 operator+ (Vector4 _v2) const { return Vector4(DirectX::XMVectorAdd(m_Vec, _v2.m_Vec)); }
	inline Vector4 operator- (Vector4 _v2) const { return Vector4(DirectX::XMVectorSubtract(m_Vec, _v2.m_Vec)); }
	inline Vector4 operator* (Vector4 _v2) const { return Vector4(DirectX::XMVectorMultiply(m_Vec, _v2.m_Vec)); }
	inline Vector4 operator/ (Vector4 _v2) const { return Vector4(DirectX::XMVectorDivide(m_Vec, _v2.m_Vec)); }
	inline Vector4 operator+ (float _v2) const { return *this + Vector4(_v2); }
	inline Vector4 operator- (float _v2) const { return *this - Vector4(_v2); }
	inline Vector4 operator* (float _v2) const { return *this * Vector4(_v2); }
	inline Vector4 operator/ (float _v2) const { return *this / Vector4(_v2); }

	inline Vector4& operator += (Vector4 _v) { *this = *this + _v; return *this; }
	inline Vector4& operator -= (Vector4 _v) { *this = *this - _v; return *this; }
	inline Vector4& operator *= (Vector4 _v) { *this = *this * _v; return *this; }
	inline Vector4& operator /= (Vector4 _v) { *this = *this / _v; return *this; }

	inline friend Vector4 operator+ (float _v1, Vector4 _v2) { return Vector4(_v1) + _v2; }
	inline friend Vector4 operator- (float _v1, Vector4 _v2) { return Vector4(_v1) - _v2; }
	inline friend Vector4 operator* (float _v1, Vector4 _v2) { return Vector4(_v1) * _v2; }
	inline friend Vector4 operator/ (float _v1, Vector4 _v2) { return Vector4(_v1) / _v2; }

private:
	DirectX::XMVECTOR m_Vec;
	DirectX::XMFLOAT4 m_VecFloat4;
};

