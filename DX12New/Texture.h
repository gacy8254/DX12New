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
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT _index = 0) const;

    /**
     * Get the DSV for the texture.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(uint32_t _mip) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;;

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

    /**
   * Check the number of bits per pixel.
   */
    size_t BitsPerPixel() const;

    static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
    static bool IsSRGBFormat(DXGI_FORMAT format);
    static bool IsBGRFormat(DXGI_FORMAT format);
    static bool IsDepthFormat(DXGI_FORMAT format);

    // Return a typeless format from the given format.
    static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);
    static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT _format);

protected:
    Texture(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, bool _isCubeMap = false, const D3D12_CLEAR_VALUE* _clearValue = nullptr);
    Texture(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc, bool _isCubeMap = false, const D3D12_CLEAR_VALUE* _clearValue = nullptr);

    virtual ~Texture();

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderAccessView;

    bool m_IsCubeMap;
};