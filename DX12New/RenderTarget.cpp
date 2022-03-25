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

D3D12_VIEWPORT RenderTarget::GetViewport(DirectX::XMFLOAT2 scale /*= { 1.0f, 1.0f }*/, DirectX::XMFLOAT2 bias /*= { 0.0f, 0.0f }*/, float minDepth /*= 0.0f*/, float maxDepth /*= 1.0f*/) const
{
	UINT64 width = 0;
	UINT height = 0;

	for (int i = AttachmentPoint::Color0; i < AttachmentPoint::Color7; ++i)
	{
		const Texture& texture = m_Textures[i];
		if (texture.IsVaild())
		{
			auto desc = texture.GetResourceDesc();
			width = std::max(width, desc.Width);
			height = std::max(height, desc.Height);
		}
	}

	D3D12_VIEWPORT viewport = {
		(width * bias.x),       // TopLeftX
		(height * bias.y),      // TopLeftY
		(width * scale.x),      // Width
		(height * scale.y),     // Height
		minDepth,               // MinDepth
		maxDepth                // MaxDepth
	};

	return viewport;
}
