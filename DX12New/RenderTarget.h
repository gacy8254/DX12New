#pragma once
#include <cstdint>
#include <vector>
#include "Texture.h"
#include <DirectXMath.h>

// Don't use scoped enums to avoid the explicit cast required to use these as 
// array indices.
enum AttachmentPoint
{
	Color0,
	Color1,
	Color2,
	Color3,
	Color4,
	Color5,
	Color6,
	Color7,
	DepthStencil,
	NumAttachmentPoints
};

class Texture;

class RenderTarget
{
public:
	//����һ���յ�RT
	RenderTarget();

	RenderTarget(const RenderTarget& _copy) = default;
	RenderTarget(RenderTarget&& _copy) = default;

	RenderTarget& operator=(const RenderTarget& _other) = default;
	RenderTarget& operator=(RenderTarget && _other) = default;

	//��һ����ͼ���ӵ���ȾĿ����
	//��ͼ���ᱻ���Ƶ���ͼ������
	void AttachTexture(AttachmentPoint _attachmentPoint, std::shared_ptr<Texture> _texture);
	std::shared_ptr<Texture> GetTexture(AttachmentPoint _attachmentPoint) const;

	//���ô�С
	void Resize(uint32_t _width, uint32_t _height);
	void Resize(DirectX::XMUINT2 _size);

	DirectX::XMUINT2 GetSize() const { return m_Size; }
	uint32_t GetWidth() const { return m_Size.x; }
	uint32_t GetHeight() const { return m_Size.y; }

	//��ȡ��ӵ���ȾĿ���ϵ���ͼ�б�
	//���������Ҫ��commandList�ڽ���ȾĿ��󶨵���Ⱦ���ߵ�����ϲ��׶�ʹ��
	const std::vector<std::shared_ptr<Texture>>& GetTextures() const;

	D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	DXGI_FORMAT GetDepthStencilFormat() const;

	D3D12_VIEWPORT GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;

	D3D12_RECT GetScissorRect();

private:
	using RenderTargetList = std::vector<std::shared_ptr<Texture>>;

	RenderTargetList m_Textures;
	DirectX::XMUINT2 m_Size;
};

