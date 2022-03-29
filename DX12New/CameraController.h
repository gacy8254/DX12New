#pragma once

#include <memory>
#include "Application.h"

class InputMap;
class BaseCamera;


class CameraController
{
public:

	enum Actions
	{
		LMB,    // Is the left-mouse button pressed?
		RMB,    // Is the right-mouse button pressed?
		MoveX,  // Move Left/right.
		MoveY,  // Move Forward/backward.
		MoveZ,  // Move Up/down.
		ZoomIn, // Zoom camera towards focal point.
		ZoomOut,// Zoom camera away from focal point.
		Pitch,  // Look up/down
		Yaw,    // Look left/right.
		Boost,  // Move/look faster
	};

	CameraController(BaseCamera& _camera);

	//重置为默认视角
	void ResetView();

	//更新相机
	void Update(UpdateEventArgs& _e);

	void SetInverseY(bool _inverseY) { m_InverseY = _inverseY; }
	bool IsInverseY() const { return m_InverseY; }

private:
	BaseCamera& m_Camera;

	std::shared_ptr<gainput::InputMap> m_KMInput;

	float m_X;
	float m_Y;
	float m_Z;
	float m_Zoom;

	float m_Pitch;
	float m_Yaw;

	float m_PreviousPitch;
	float m_PreviousYaw;

	bool m_InverseY;
};

