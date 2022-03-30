#include "CameraController.h"
#include <cmath>
#include <algorithm>
#include <utility>

#include "BaseCamera.h"
#include "Transform.h"

inline double Lerp(float _v1, float _v2, float _t)
{
	return _v1 + _t * (_v2 - _v1);
}

inline void Smooth(float& _v1, float& _v2, float _deltaTime)
{
	float x;
	if (std::fabsf(_v1) < std::fabsf(_v2))
	{
		x = Lerp(_v2, _v1, std::powf(0.6, _deltaTime * 60.0f));
	}
	else
	{
		x = Lerp(_v2, _v1, std::powf(0.8, _deltaTime * 60.0f));
	}

	_v1 = x;
	_v2 = x;
}

CameraController::CameraController(BaseCamera& _camera)
	:
	m_Camera(_camera),
	m_X(0.0f),
	m_Y(1.0f),
	m_Z(0.0f),
	m_Pitch(0.0f),
	m_Yaw(0.0f),
	m_PreviousPitch(0.0f),
	m_PreviousYaw(0.0f)
{
	auto& app = Application::Get();

	m_KMInput = app.CreateInputMap("CameraController (Keyboard/Mouse)");

	auto keyboard = app.GetKeyboardId();
	auto mouse = app.GetMouseId();

	//映射键盘事件
	m_KMInput->MapFloat(MoveX, keyboard, gainput::KeyD, 0.0f, 1.0f);
	m_KMInput->MapFloat(MoveX, keyboard, gainput::KeyA, 0.0f, -1.0f);
	m_KMInput->MapFloat(MoveY, keyboard, gainput::KeyE, 0.0f, 1.0f);
	m_KMInput->MapFloat(MoveY, keyboard, gainput::KeyQ, 0.0f, -1.0f);
	m_KMInput->MapFloat(MoveZ, keyboard, gainput::KeyW, 0.0f, 1.0f);
	m_KMInput->MapFloat(MoveZ, keyboard, gainput::KeyS, 0.0f, -1.0f);
	m_KMInput->MapFloat(Pitch, keyboard, gainput::KeyUp, 0.0f, 1.0f);
	m_KMInput->MapFloat(Pitch, keyboard, gainput::KeyDown, 0.0f, -1.0f);
	m_KMInput->MapFloat(Yaw, keyboard, gainput::KeyLeft, 0.0f, 1.0f);
	m_KMInput->MapFloat(Yaw, keyboard, gainput::KeyRight, 0.0f, -1.0f);
	m_KMInput->MapBool(Boost, keyboard, gainput::KeyShiftL);
	m_KMInput->MapBool(Boost, keyboard, gainput::KeyShiftR);

	//映射鼠标事件
	m_KMInput->MapBool(LMB, mouse, gainput::MouseButtonLeft);
	m_KMInput->MapBool(RMB, mouse, gainput::MouseButtonRight);
	m_KMInput->MapFloat(Pitch, mouse, gainput::MouseAxisY);
	m_KMInput->MapFloat(Yaw, mouse, gainput::MouseAxisX);
	m_KMInput->MapBool(ZoomIn, mouse, gainput::MouseButtonWheelUp);
	m_KMInput->MapBool(ZoomOut, mouse, gainput::MouseButtonWheelDown);

	m_KMInput->SetUserButtonPolicy(Pitch, gainput::InputMap::UBP_MAX);
	m_KMInput->SetUserButtonPolicy(Yaw, gainput::InputMap::UBP_MAX);

	ResetView();
}

void CameraController::ResetView()
{
	m_X = m_Y = m_Z = m_PreviousYaw = m_PreviousPitch = 0.0f;
	m_Pitch = 15.0f;
	m_Yaw = 90.0f;
	m_Zoom = 10.0f;

	Vector4 rotation = Transform::QuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(m_Pitch), DirectX::XMConvertToRadians(m_Yaw), 0.0f);
	m_Camera.SetRotation(rotation);
	m_Camera.SetTranslation(Vector4(0, 0, -m_Zoom, 0.0f));
	m_Camera.SetFocalPoint(Vector4(0, 0, 0, 1.0f));
}

void CameraController::Update(UpdateEventArgs& _e)
{
	const float MOVE_SPEED = 10.0f;
	const float LOOK_SENSITIVITY = 10.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	float speedScale = m_KMInput->GetBool(Boost) ? 2.0f : 1.0f;
	float rotationScale = m_KMInput->GetBool(Boost) ? 2.0f : 1.0f;

	float x = m_KMInput->GetFloat(MoveX) * MOVE_SPEED * speedScale * _e.DeltaTime;
	float y = m_KMInput->GetFloat(MoveY) * MOVE_SPEED * speedScale * _e.DeltaTime;
	float z = m_KMInput->GetFloat(MoveZ) * MOVE_SPEED * speedScale * _e.DeltaTime;
	float zoom = m_KMInput->GetBool(ZoomOut) ? 1.0f : 0.0f;
	zoom -= m_KMInput->GetBool(ZoomIn) ? 1.0f : 0.0f;

	Smooth(m_X, x, _e.DeltaTime);
	Smooth(m_Y, y, _e.DeltaTime);
	Smooth(m_Z, z, _e.DeltaTime);
	
	float pitch = 0.0f;
	float yaw = 0.0f;
	if (m_KMInput->GetBool(LMB))
	{
		pitch = m_KMInput->GetFloatDelta(Pitch) * MOUSE_SENSITIVITY * rotationScale;
		yaw = m_KMInput->GetFloatDelta(Yaw) * MOUSE_SENSITIVITY * rotationScale;
	}

	m_Pitch -= pitch * (m_InverseY ? 1.0f : -1.0f);
	m_Pitch = std::clamp(m_Pitch, -90.0f, 90.0f);

	m_Yaw += yaw;

	Vector4 focalPoint(x, y, z, 1);
	m_Camera.MoveFocalPoint(focalPoint, Space::Local);

	m_Zoom += zoom;
	m_Zoom = m_Zoom > 0.0f ? m_Zoom :  0.0f;
	Vector4 translation(0, 0, -m_Zoom, 1);
	m_Camera.SetTranslation(translation);

	Vector4 rotation = Transform::QuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(m_Pitch), DirectX::XMConvertToRadians(m_Yaw), 0.0f);
	m_Camera.SetRotation(rotation);
}
