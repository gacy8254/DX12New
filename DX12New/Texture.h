#pragma once

#include "Resource.h"
#include "DescriptorAllocation.h"
#include "TextureUsage.hpp"

#include "d3dx12.h"

#include <mutex>
#include <unordered_map>

class Texture : public Resource
{
public:
    TextureUsage GetTextureUsage() const
    {
        return m_TextureUsage;
    }

    void SetTextureUsage(TextureUsage textureUsage)
    {
        m_TextureUsage = textureUsage;
    }

    /**
     * Resize the texture.
     */
    void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

    /**
     * Create SRV and UAVs for the resource.
     */
    virtual void CreateViews();

    /**
     * Get the RTV for the texture.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

    /**
     * Get the DSV for the texture.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    bool CheckSRVSupport()
    {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
    }

    bool CheckRTVSupport()
    {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
    }

    bool CheckUAVSupport()
    {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
            CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
            CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }

    bool CheckDSVSupport()
    {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
    }

    bool HasAlpha() const;

    static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
    static bool IsSRGBFormat(DXGI_FORMAT format);
    static bool IsBGRFormat(DXGI_FORMAT format);
    static bool IsDepthFormat(DXGI_FORMAT format);

    // Return a typeless format from the given format.
    static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT _format);

protected:
    Texture(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, const D3D12_CLEAR_VALUE* _clearValue = nullptr);
    Texture(Device& _device, const D3D12_RESOURCE_DESC* _resourceDesc, const D3D12_CLEAR_VALUE* _clearValue = nullptr);

    virtual ~Texture();

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderAccessView;

    TextureUsage m_TextureUsage;
};