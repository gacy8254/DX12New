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
	assert(!m_pWindow && "在使用Game::Destroy()之前被释放");
}

bool Game::Init()
{
	//检查DX数学库的支持
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "检查数学库支持失败", "错误", MB_OK | MB_ICONERROR);
		return false;
	}

	//调用Application类创建窗口
	m_pWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_VSync);
	//注册回调函数
	m_pWindow->RegisterCallBacks(shared_from_this());
	//显示窗口
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
	//当绑定的窗口被销毁时，将所有相关联的资源释放掉
	UnLoadContent();
}
