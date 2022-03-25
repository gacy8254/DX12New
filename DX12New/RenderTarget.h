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
	void AttachTexture(AttachmentPoint _attachmentPoint, const Texture& _texture);
	const Texture& GetTexture(AttachmentPoint _attachmentPoint) const;

	//���ô�С
	void Resize(uint32_t _width, uint32_t _height);

	//��ȡ��ӵ���ȾĿ���ϵ���ͼ�б�
	//���������Ҫ��commandList�ڽ���ȾĿ��󶨵���Ⱦ���ߵ�����ϲ��׶�ʹ��
	const std::vector<Texture>& GetTextures() const;

	D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	DXGI_FORMAT GetDepthStencilFormat() const;

	D3D12_VIEWPORT GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;

private:
	std::vector<Texture> m_Textures;
};

