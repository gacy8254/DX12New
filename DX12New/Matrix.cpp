#include "Matrix.h"

Matrix3::Matrix3(Tag _tag)
{
	switch (_tag)
	{
	case ONE:
		m_Mat[0] = Vector3(Tag::X), m_Mat[1] = Vector3(Tag::Y), m_Mat[2] = Vector3(Tag::Z);
		break;
	case ZERO:
		m_Mat[0] = m_Mat[1] = m_Mat[2] = Vector3(Tag::ZERO);
		break;
	}
}

Matrix4::Matrix4(Tag _tag)
{
	switch (_tag)
	{
	case ONE:
		m_Mat = DirectX::XMMatrixIdentity();
		break;
	case ZERO:
		m_Mat.r[0] = m_Mat.r[1] = m_Mat.r[2] = m_Mat.r[3] = SplatZero();
		break;
	}
}
