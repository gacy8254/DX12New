#include "Lesson3.h"
#include "Application.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "helpers.h"
#include "Light.h"
#include "Material.h"
#include "Window.h"

#include <wrl.h>
using namespace Microsoft::WRL;

#include "d3dx12.h"
#include <d3dcompiler.h>
#include <DirectXColors.h>
using namespace DirectX;

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

struct Mat
{
	XMMATRIX ModelMatrix;
	XMMATRIX ModelViewMatrix;
	XMMATRIX InverseTransposeModelMatrix;
	XMMATRIX ModelViewProjMatrix;
};

struct LightProperties
{
	uint32_t NumPointLights;
	uint32_t NumSpotLights;
};

enum RootParameters
{
	MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
	MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
	LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );
	PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
	SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
	Textures,           // Texture2D DiffuseTexture : register( t2 );
	NumRootParameters

};

// Builds a look-at (world) matrix from a point, up and direction vectors.
XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR _position, FXMVECTOR _direction, FXMVECTOR  _up)
{
	assert(!XMVector3Equal(_direction, XMVectorZero()));
	assert(!XMVector3IsInfinite(_direction));
	assert(!XMVector3Equal(_up, XMVectorZero()));
	assert(!XMVector3IsInfinite(_up));

	XMVECTOR R2 = XMVector3Normalize(_direction);
	XMVECTOR R0 = XMVector3Cross(_up, R2);
	R0 = XMVector3Normalize(R0);

	XMVECTOR R1 = XMVector3Cross(R2, R0);

	XMMATRIX M(R0, R1, R2, _position);

	return M;
}

Lesson3::Lesson3(const std::wstring& _name, int _width, int _height, bool _vSync)
	:super(_name, _width, _height, _vSync),
	m_Rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0F, 0.0F, static_cast<float>(_width), static_cast<float>(_height))),
	m_Forward(0)
	, m_Backward(0)
	, m_Left(0)
	, m_Right(0)
	, m_Up(0)
	, m_Down(0)
	, m_Pitch(0)
	, m_Yaw(0)
	, m_AnimaeLights(false)
	, m_Shift(false)
	, m_Width(0)
	, m_Height(0)
{
	XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_Camera.SetLookAt(cameraPos, cameraTarget, cameraUp);

	m_pAlignedCameraDate = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);

	m_pAlignedCameraDate->m_InitialCamPos = m_Camera.GetTranslation();
	m_pAlignedCameraDate->m_InitialCamRot = m_Camera.GetRotation();
}

Lesson3::~Lesson3()
{
	_aligned_free(m_pAlignedCameraDate);
}

bool Lesson3::LoadContent()
{
	return true;
}

void Lesson3::UnLoadContent()
{

}

void Lesson3::OnUpdate(UpdateEventArgs& _e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(_e);

	totalTime += _e.ElapsedTime;
	frameCount++;

	if (totalTime > 1.0f)
	{
		double fps = frameCount / totalTime;

		char buffer[500];
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		WCHAR wszClassName[256];
		memset(wszClassName, 0, sizeof(wszClassName));
		MultiByteToWideChar(CP_ACP, 0, buffer, static_cast<int>(strlen(buffer) + 1), wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));
		OutputDebugString(wszClassName);

		frameCount = 0;
		totalTime = 0.0f;
	}
}

void XM_CALLCONV ComputerMatrices(FXMMATRIX _model, CXMMATRIX _view, CXMMATRIX _viewProj, Mat& _mat)
{
	_mat.ModelMatrix = _model;
	_mat.ModelViewMatrix = _model * _view;
	_mat.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, _mat.ModelViewMatrix));
	_mat.ModelViewProjMatrix = _model * _viewProj;
}

void Lesson3::OnRender(RenderEventArgs& _e)
{

}

static bool g_AllowFullscreenToggle = true;

void Lesson3::OnKeyPressed(KeyEventArgs& _e)
{
	super::OnKeyPressed(_e);

	switch (_e.key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;
	case KeyCode::Enter:
		if (_e.alt)
		{
	case KeyCode::F11:
		m_pWindow->ToggleFullScreen();
		break;
		}
	case KeyCode::V:
		m_pWindow->ToggleVSync();
		break;
	}
}

void Lesson3::OnKeyReleased(KeyEventArgs& _e)
{

}

void Lesson3::OnMouseMoved(MouseMotionEventArgs& _e)
{

}

void Lesson3::OnMouseButtonPressed(MouseButtonEventArgs& _e)
{

}

void Lesson3::OnMouseButtonReleased(MouseButtonEventArgs& _e)
{

}

void Lesson3::OnMouseWheel(MouseWheelEventArgs& _e)
{
}

void Lesson3::OnResize(ResizeEventArgs& _e)
{
	super::OnResize(_e);

	if (_e.Width != GetWidth() || _e.Height != GetHeight())
	{
		m_Width = std::max(1, _e.Width);
		m_Height = std::max(1, _e.Height);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

		float aspect = m_Width / (float)m_Height;
		m_Camera.SetProjection(45.0f, aspect, 0.1f, 1000.0f);

		m_RenderTarget.Resize(m_Width, m_Height);
	}
}

void Lesson3::OnWindowDestroy()
{

}
