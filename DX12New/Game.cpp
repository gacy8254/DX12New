#include "Game.h"
#include "D3D12LibPCH.h"
#include "Application.h"
#include "Window.h"

Game::Game(const std::wstring& _name, int _width, int _height, bool _vSync)
	:m_Name(_name), m_Width(_width), m_Height(_height), m_VSync(_vSync)
{

}

Game::~Game()
{
	assert(!m_pWindow && "��ʹ��Game::Destroy()֮ǰ���ͷ�");
}

bool Game::Init()
{
	//���DX��ѧ���֧��
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "�����ѧ��֧��ʧ��", "����", MB_OK | MB_ICONERROR);
		return false;
	}

	//����Application�ഴ������
	m_pWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_VSync);
	//ע��ص�����
	m_pWindow->RegisterCallBacks(shared_from_this());
	//��ʾ����
	m_pWindow->Show();

	return true;
}

void Game::Destroy()
{
	Application::Get().DestroyWindow(m_pWindow);
	m_pWindow.reset();
}

void Game::OnUpdate(UpdateEventArgs& _e)
{

}

//void Game::OnRender(RenderEventArgs& _e)
//{
//
//}

void Game::OnKeyPressed(KeyEventArgs& _e)
{

}

void Game::OnKeyReleased(KeyEventArgs& _e)
{

}

void Game::OnMouseMoved(MouseMotionEventArgs& _e)
{

}

void Game::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{

}

void Game::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{

}

void Game::OnMouseWheel(MouseWheelEventArgs& _e)
{

}

void Game::OnResize(ResizeEventArgs& _e)
{

}

void Game::OnWindowDestroy()
{
	//���󶨵Ĵ��ڱ�����ʱ�����������������Դ�ͷŵ�
	UnLoadContent();
}
