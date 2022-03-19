#pragma once
#include "KeyCodes.h"
#include <stdint.h>

class EventArgs
{
public:
	EventArgs(){}
};

class KeyEventArgs : public EventArgs
{
public:
	enum KeyState
	{
		Released = 0,
		Pressed = 1
	};

	typedef EventArgs base;
	KeyEventArgs(KeyCode::Key _key, unsigned int _c, KeyState _state, bool _control, bool _shift, bool _alt)
		:key(_key), Char(_c), state(_state), control(_control), shift(_shift), alt(_alt)
	{}

	KeyCode::Key key;
	unsigned int Char;
	KeyState state;
	bool control;
	bool shift;
	bool alt;
};

class MouseMotionEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	MouseMotionEventArgs(bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	bool LeftButton;    // Is the left mouse button down?
	bool MiddleButton;  // Is the middle mouse button down?
	bool RightButton;   // Is the right mouse button down?
	bool Control;       // Is the CTRL key down?
	bool Shift;         // Is the Shift key down?

	int X;              // The X-position of the cursor relative to the upper-left corner of the client area.
	int Y;              // The Y-position of the cursor relative to the upper-left corner of the client area.
	int RelX;           // How far the mouse moved since the last event.
	int RelY;           // How far the mouse moved since the last event.
};

class MouseButtonEventArgs : public EventArgs
{
public:
	enum MouseButton
	{
		None = 0,
		Left = 1,
		Right = 2,
		Middel = 3
	};
	enum ButtonState
	{
		Released = 0,
		Pressed = 1
	};

	typedef EventArgs base;
	MouseButtonEventArgs(MouseButton buttonID, ButtonState state, bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: Button(buttonID)
		, State(state)
		, LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	MouseButton Button; // The mouse button that was pressed or released.
	ButtonState State;  // Was the button pressed or released?
	bool LeftButton;    // Is the left mouse button down?
	bool MiddleButton;  // Is the middle mouse button down?
	bool RightButton;   // Is the right mouse button down?
	bool Control;       // Is the CTRL key down?
	bool Shift;         // Is the Shift key down?

	int X;              // The X-position of the cursor relative to the upper-left corner of the client area.
	int Y;              // The Y-position of the cursor relative to the upper-left corner of the client area.
};

class MouseWheelEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	MouseWheelEventArgs(float wheelDelta, bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: WheelDelta(wheelDelta)
		, LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	float WheelDelta;   // How much the mouse wheel has moved. A positive value indicates that the wheel was moved to the right. A negative value indicates the wheel was moved to the left.
	bool LeftButton;    // Is the left mouse button down?
	bool MiddleButton;  // Is the middle mouse button down?
	bool RightButton;   // Is the right mouse button down?
	bool Control;       // Is the CTRL key down?
	bool Shift;         // Is the Shift key down?

	int X;              // The X-position of the cursor relative to the upper-left corner of the client area.
	int Y;              // The Y-position of the cursor relative to the upper-left corner of the client area.

};

class ResizeEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	ResizeEventArgs(int width, int height)
		: Width(width)
		, Height(height)
	{}

	// The new width of the window
	int Width;
	// The new height of the window.
	int Height;

};

class UpdateEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	UpdateEventArgs(double fDeltaTime, double fTotalTime, uint64_t frameCount)
		: ElapsedTime(fDeltaTime)
		, TotalTime(fTotalTime)
		,FrameCount(frameCount)
	{}

	double ElapsedTime;
	double TotalTime;
	uint64_t FrameCount;
};

class RenderEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	RenderEventArgs(double fDeltaTime, double fTotalTime, uint64_t frameCount)
		: ElapsedTime(fDeltaTime)
		, TotalTime(fTotalTime)
		, FrameCount(frameCount)
	{}

	double ElapsedTime;
	double TotalTime;
	uint64_t FrameCount;
};

class UserEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	UserEventArgs(int code, void* data1, void* data2)
		: Code(code)
		, Data1(data1)
		, Data2(data2)
	{}

	int     Code;
	void* Data1;
	void* Data2;
};