#pragma once
#include "Vector.h"
#include "Matrix.h"

namespace Transform
{
	//����۲����,��������ϵ
	Matrix4 MatrixLookATLH(Vector4 _eye, Vector4 _target, Vector4 _up);

	//����͸�Ӿ���
	Matrix4 MatrixPerspectiveForLH(float _fov, float _aspect, float _zNear, float _zFar);

	//���ݾ������һ����ת��Ԫ��
	Vector4 QuaternionRotationMatrix(Matrix4 _mat);

	//����������
	Matrix4 InverseMatrix(Vector4* _vec, Matrix4 _mat);

	//��ת����
	Vector4 Vector4Rotate(Vector4 _translation, Vector4 _rotation);

	//������Ԫ���ĳ˻�
	Vector4 QuaternionMultiply(Vector4 _quaternion, Vector4 _ratation);

	//��������ת��
	Matrix4 MatrixTranspose(Matrix4 _mat);

	//������Ԫ��������ת����
	Matrix4 MatrixRotationQuaternion(Vector4 _quaternion);

	//������������λ�ƾ���
	Matrix4 MatrixTranslateFromVector(Vector4 _v);

	//����rollPitchYaw������ת��Ԫ��
	Vector4 QuaternionRotationRollPitchYaw(float _pitch, float _yaw, float _roll);
	Vector4 QuaternionRotationRollPitchYaw(Vector4 _radians);

	//�������ž���
	Matrix4 MatrixScaling(float _x, float _y, float _z);

	//�������ž���
	Matrix4 MatrixScaling(Vector4 _scale);

	//�������������Ƿ����
	bool Vector3Equal(Vector4 _v1, Vector4 _v2);

	//����һ�������Ƿ���������
	bool Vector3IsInfinite(Vector4 _v);

	//������һ��
	Vector4 Vector3Normalize(Vector4 _v);

	//���
	Vector4 Vector3Cross(Vector4 _v1, Vector4 _v2);

	//��ת����X��
	Matrix4 MatrixRotationX(float _angle);

	//ʹ�þ��������ת���ű仯,���Ծ����λ�Ʋ���
	Vector4 Vector3TransformNormal(Vector4 _v, Matrix4 _mat);

	//���Ƕ���ת��Ϊ����
	Vector4 ConvertToRadians(Vector4 _angle);
}

