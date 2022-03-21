#include "RenderTarget.h"

#include "D3D12LibPCH.h"

RenderTarget::RenderTarget()
	:m_Textures(AttachmentPoint::NumAttachmentPoints)
{
}

void RenderTarget::AttachTexture(AttachmentPoint _attachmentPoint, const Texture& _texture)
{
	m_Textures[_attachmentPoint] = _texture;
}

const Texture& RenderTarget::GetTexture(AttachmentPoint _attachmentPoint) const
{
	return m_Textures[_attachmentPoint];
}

void RenderTarget::Resize(uint32_t _width, uint32_t _height)
{
	for (auto& texture : m_Textures)
	{
		texture.Resize(_width, _height);
	}
}

const std::vector<Texture>& RenderTarget::GetTextures() const
{
	return m_Textures;
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
	D3D12_RT_FORMAT_ARRAY rtFormats = {};

	for (int i = AttachmentPoint::Color0; i < AttachmentPoint::Color7; ++i)
	{
		const Texture& texture = m_Textures[i];
		if (texture.IsVaild())
		{
			rtFormats.RTFormats[rtFormats.NumRenderTargets++] = texture.GetResourceDesc().Format;
		}
	}

	return rtFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	const Texture& dsTexture = m_Textures[AttachmentPoint::DepthStencil];
	if (dsTexture.IsVaild())
	{
		format = dsTexture.GetResourceDesc().Format;
	}
	return format;
}
