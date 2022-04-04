#pragma once
#include "Vector.h"
#include "Matrix.h"

namespace Transform
{
	//计算观察矩阵,左手坐标系
	Matrix4 MatrixLookATLH(Vector4 _eye, Vector4 _target, Vector4 _up);

	//计算透视矩阵
	Matrix4 MatrixPerspectiveForLH(float _fov, float _aspect, float _zNear, float _zFar);

	//根据矩阵计算一个旋转四元数
	Vector4 QuaternionRotationMatrix(Matrix4 _mat);

	//计算矩阵的逆
	Matrix4 InverseMatrix(Vector4* _vec, Matrix4 _mat);

	//旋转向量
	Vector4 Vector4Rotate(Vector4 _translation, Vector4 _rotation);

	//计算四元数的乘积
	Vector4 QuaternionMultiply(Vector4 _quaternion, Vector4 _ratation);

	//计算矩阵的转置
	Matrix4 MatrixTranspose(Matrix4 _mat);

	//根据四元数计算旋转矩阵
	Matrix4 MatrixRotationQuaternion(Vector4 _quaternion);

	//根据向量计算位移矩阵
	Matrix4 MatrixTranslateFromVector(Vector4 _v);

	//根据rollPitchYaw计算旋转四元数
	Vector4 QuaternionRotationRollPitchYaw(float _pitch, float _yaw, float _roll);
	Vector4 QuaternionRotationRollPitchYaw(Vector4 _radians);

	//计算缩放矩阵
	Matrix4 MatrixScaling(float _x, float _y, float _z);

	//计算缩放矩阵
	Matrix4 MatrixScaling(Vector4 _scale);

	//计算两个向量是否相等
	bool Vector3Equal(Vector4 _v1, Vector4 _v2);

	//计算一个向量是否是无穷大的
	bool Vector3IsInfinite(Vector4 _v);

	//向量归一化
	Vector4 Vector3Normalize(Vector4 _v);

	//叉乘
	Vector4 Vector3Cross(Vector4 _v1, Vector4 _v2);

	//旋转矩阵X轴
	Matrix4 MatrixRotationX(float _angle);

	//使用矩阵进行旋转缩放变化,忽略矩阵的位移部分
	Vector4 Vector3TransformNormal(Vector4 _v, Matrix4 _mat);

	//将角度制转换为弧度
	Vector4 ConvertToRadians(Vector4 _angle);
}

