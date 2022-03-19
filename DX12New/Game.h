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

	//��������
	virtual bool Init();

	virtual bool LoadContent() = 0;

	virtual void UnLoadContent() = 0;

	//���ٴ���
	virtual void Destroy();

	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }

protected:
	friend class Window;

	//������Ϸ�߼�
	virtual void OnUpdate(UpdateEventArgs& _e);

	//������Ⱦ
	virtual void OnRender(RenderEventArgs& _e);

	//���������¼�
	virtual void OnKeyPressed(KeyEventArgs& _e);
	virtual void OnKeyReleased(KeyEventArgs& _e);
	virtual void OnMouseMoved(MouseMotionEventArgs& _e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& _e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& _e);
	virtual void OnMouseWheel(MouseWheelEventArgs& _e);
	virtual void OnResize(ResizeEventArgs& _e);

	virtual void OnWindowDestroy();

	//����ʵ��
	std::shared_ptr<Window> m_pWindow;

private:
	std::wstring m_Name;
	int m_Width;
	int m_Height;
	bool m_VSync;






};

