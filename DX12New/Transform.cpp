#include "Transform.h"
using namespace DirectX;

Matrix4 Transform::MatrixLookATLH(Vector4 _eye, Vector4 _target, Vector4 _up)
{
	return Matrix4(XMMatrixLookAtLH(_eye, _target, _up));
}

Matrix4 Transform::MatrixPerspectiveForLH(float _fov, float _aspect, float _zNear, float _zFar)
{
	return Matrix4(XMMatrixPerspectiveFovLH(_fov, _aspect, _zNear, _zFar));
}

Vector4 Transform::QuaternionRotationMatrix(Matrix4 _mat)
{
	return Vector4(XMQuaternionRotationMatrix(XMMatrixTranspose(_mat)));
}

Matrix4 Transform::InverseMatrix(Vector4* _vec, Matrix4 _mat)
{
	return Matrix4(XMMatrixInverse(_vec->GetPoint(), _mat));
}

Vector4 Transform::Vector4Rotate(Vector4 _translation, Vector4 _rotation)
{
	return Vector4(XMVector3Rotate(_translation, _rotation));
}

Vector4 Transform::QuaternionMultiply(Vector4 _quaternion, Vector4 _ratation)
{
	return Vector4(XMQuaternionMultiply(_quaternion, _ratation));
}

Matrix4 Transform::MatrixTranspose(Matrix4 _mat)
{
	return Matrix4(XMMatrixTranspose(_mat));
}

Matrix4 Transform::MatrixRotationQuaternion(Vector4 _quaternion)
{
	return Matrix4(XMMatrixRotationQuaternion(_quaternion));
}

Matrix4 Transform::MatrixTranslateFromVector(Vector4 _v)
{
	return Matrix4(XMMatrixTranslationFromVector(_v));
}

Vector4 Transform::QuaternionRotationRollPitchYaw(float _pitch, float _yaw, float _roll)
{
	return Vector4(XMQuaternionRotationRollPitchYaw(_pitch, _yaw, _roll));
}

Vector4 Transform::QuaternionRotationRollPitchYaw(Vector4 _radians)
{
	return Vector4(XMQuaternionRotationRollPitchYaw(_radians.GetX(), _radians.GetY(), _radians.GetZ()));
}

Matrix4 Transform::MatrixScaling(float _x, float _y, float _z)
{
	return Matrix4(XMMatrixScaling(_x, _y, _z));
}

Matrix4 Transform::MatrixScaling(Vector4 _scale)
{
	return Matrix4(XMMatrixScalingFromVector(_scale));
}

bool Transform::Vector3Equal(Vector4 _v1, Vector4 _v2)
{
	return XMVector3Equal(_v1, _v2);
}

bool Transform::Vector3IsInfinite(Vector4 _v)
{
	return XMVector3IsInfinite(_v);
}

Vector4 Transform::Vector3Normalize(Vector4 _v)
{
	return Vector4(XMVector3Normalize(_v));
}

Vector4 Transform::Vector3Cross(Vector4 _v1, Vector4 _v2)
{
	return Vector4(XMVector3Cross(_v1, _v2));
}

Matrix4 Transform::MatrixRotationX(float _angle)
{
	return Matrix4(XMMatrixRotationX(_angle));
}

Matrix4 Transform::MatrixRotationY(float _angle)
{
	return Matrix4(XMMatrixRotationY(_angle));
}

Matrix4 Transform::MatrixRotationZ(float _angle)
{
	return Matrix4(XMMatrixRotationZ(_angle));
}

Vector4 Transform::Vector3TransformNormal(Vector4 _v, Matrix4 _mat)
{
	return Vector4(XMVector3TransformNormal(_v, _mat));
}

Vector4 Transform::ConvertToRadians(Vector4 _angle)
{
	return Vector4(DirectX::XMConvertToRadians(_angle.GetX()),
		DirectX::XMConvertToRadians(_angle.GetY()),
		DirectX::XMConvertToRadians(_angle.GetZ()),
		0);
}

