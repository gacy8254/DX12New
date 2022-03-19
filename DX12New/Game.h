#pragma once
#include "Event.h"

#include <memory>
#include <string>

class Window;

class Game : public std::enable_shared_from_this<Game>
{
public:
	Game(const std::wstring& _name, int _width, int _height, bool _vSync);
	virtual ~Game();

	//创建窗口
	virtual bool Init();

	virtual bool LoadContent() = 0;

	virtual void UnLoadContent() = 0;

	//销毁窗口
	virtual void Destroy();

	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }

protected:
	friend class Window;

	//更新游戏逻辑
	virtual void OnUpdate(UpdateEventArgs& _e);

	//进行渲染
	virtual void OnRender(RenderEventArgs& _e);

	//各种输入事件
	virtual void OnKeyPressed(KeyEventArgs& _e);
	virtual void OnKeyReleased(KeyEventArgs& _e);
	virtual void OnMouseMoved(MouseMotionEventArgs& _e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e);
	virtual void OnMouseWheel(MouseWheelEventArgs& _e);
	virtual void OnResize(ResizeEventArgs& _e);

	virtual void OnWindowDestroy();

	//窗口实例
	std::shared_ptr<Window> m_pWindow;

private:
	std::wstring m_Name;
	int m_Width;
	int m_Height;
	bool m_VSync;






};

