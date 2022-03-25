#include "Vector.h"

Vector3::Vector3(Vector4 _v) :m_Vec((DirectX::XMVECTOR)_v)
{

}

Vector3::Vector3(Tag _tag)
{
	switch (_tag)
	{
	case ONE:
		m_Vec = DirectX::XMVectorSplatOne();
		break;
	case ZERO:
		m_Vec = DirectX::XMVectorZero();
		break;
	case X:
		m_Vec = DirectX::g_XMIdentityR0;
		break;
	case Y:
		m_Vec = DirectX::g_XMIdentityR1;
		break;
	case Z:
		m_Vec = DirectX::g_XMIdentityR2;
		break;
	case W:
		m_Vec = DirectX::g_XMIdentityR3;
		break;
	}
}

Vector4::Vector4(Tag _tag)
{
	switch (_tag)
	{
	case ONE:
		m_Vec = DirectX::XMVectorSplatOne();
		break;
	case ZERO:
		m_Vec = DirectX::XMVectorZero();
		break;
	case X:
		m_Vec = DirectX::g_XMIdentityR0;
		break;
	case Y:
		m_Vec = DirectX::g_XMIdentityR1;
		break;
	case Z:
		m_Vec = DirectX::g_XMIdentityR2;
		break;
	case W:
		m_Vec = DirectX::g_XMIdentityR3;
		break;
	}
}
