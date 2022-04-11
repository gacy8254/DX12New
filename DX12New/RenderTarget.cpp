#include "RenderTarget.h"

#include "D3D12LibPCH.h"

RenderTarget::RenderTarget()
	:m_Textures(AttachmentPoint::NumAttachmentPoints), m_Size(0, 0)
{
}

void RenderTarget::AttachTexture(AttachmentPoint _attachmentPoint, std::shared_ptr<Texture> _texture)
{
	m_Textures[_attachmentPoint] = _texture;

	if (_texture && _texture->GetResource())
	{
		auto desc = _texture->GetResourceDesc();

		m_Size.x = static_cast<uint32_t>(desc.Width);
		m_Size.y = static_cast<uint32_t>(desc.Height);
	}
}

std::shared_ptr<Texture> RenderTarget::GetTexture(AttachmentPoint _attachmentPoint) const
{
	return m_Textures[_attachmentPoint];
}

void RenderTarget::Resize(uint32_t _width, uint32_t _height)
{
	Resize(DirectX::XMUINT2(_width, _height));
}

void RenderTarget::Resize(DirectX::XMUINT2 _size)
{
	m_Size = _size;
	for (auto tex : m_Textures)
	{
		if (tex)
		{
			tex->Resize(m_Size.x, m_Size.y);
		}
	}
}

const std::vector<std::shared_ptr<Texture>>& RenderTarget::GetTextures() const
{
	return m_Textures;
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
	D3D12_RT_FORMAT_ARRAY rtFormats = {};

	for (int i = AttachmentPoint::Color0; i < AttachmentPoint::Color7; ++i)
	{
		auto texture = m_Textures[i];
		if (texture)
		{
			rtFormats.RTFormats[rtFormats.NumRenderTargets++] = texture->GetResourceDesc().Format;
		}
	}

	return rtFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	auto dsTexture = m_Textures[AttachmentPoint::DepthStencil];
	if (dsTexture)
	{
		format = dsTexture->GetResourceDesc().Format;
	}
	return format;
}

D3D12_VIEWPORT RenderTarget::GetViewport(DirectX::XMFLOAT2 scale /*= { 1.0f, 1.0f }*/, 
	DirectX::XMFLOAT2 bias /*= { 0.0f, 0.0f }*/, 
	float minDepth /*= 0.0f*/, float maxDepth /*= 1.0f*/) const
{
	UINT64 width = 0;
	UINT height = 0;

	for (int i = AttachmentPoint::Color0; i < AttachmentPoint::Color7; ++i)
	{
		auto texture = m_Textures[i];
		if (texture)
		{
			auto desc = texture->GetResourceDesc();
			width = std::max(width, desc.Width);
			height = std::max(height, desc.Height);
		}
	}

	if (width == 0 || height == 0)
	{
		auto texture = m_Textures[8];
		if (texture)
		{
			auto desc = texture->GetResourceDesc();
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

D3D12_RECT RenderTarget::GetScissorRect()
{
	UINT64 width = 0;
	UINT height = 0;

	for (int i = AttachmentPoint::Color0; i < AttachmentPoint::Color7; ++i)
	{
		auto texture = m_Textures[i];
		if (texture)
		{
			auto desc = texture->GetResourceDesc();
			width = std::max(width, desc.Width);
			height = std::max(height, desc.Height);
		}
	}

	if (width == 0 || height == 0)
	{
		auto texture = m_Textures[8];
		if (texture)
		{
			auto desc = texture->GetResourceDesc();
			width = std::max(width, desc.Width);
			height = std::max(height, desc.Height);
		}
	}


	D3D12_RECT resc = { 0, 0, (int)width, (int)height };

	return resc;
}

DXGI_SAMPLE_DESC RenderTarget::GetSamplerDesc() const
{
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
	for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::Color7; ++i)
	{
		auto texture = m_Textures[i];
		if (texture)
		{
			sampleDesc = texture->GetResourceDesc().SampleDesc;
			break;
		}
	}

	return sampleDesc;
}
