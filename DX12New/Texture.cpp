#include "D3D12LibPCH.h"
#include "Texture.h"
#include "helpers.h"
#include "ResourceStateTracker.h"
#include "ShaderResourceView.h"
#include "UnorderedAccessView.h"
#include "DescriptorAllocation.h"
#include <iostream>

Texture::Texture(Device& _device, Microsoft::WRL::ComPtr<ID3D12Resource> _resource, bool _isCubeMap, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
	:Resource(_device, _resource, _clearValue),
	m_IsCubeMap(_isCubeMap)
{
	CreateViews();
}

Texture::Texture(Device& _device, const D3D12_RESOURCE_DESC& _resourceDesc, bool _isCubeMap, const D3D12_CLEAR_VALUE* _clearValue /*= nullptr*/)
	: Resource(_device, _resourceDesc, _clearValue),
	m_IsCubeMap(_isCubeMap)
{
	CreateViews();
}

Texture::~Texture()
{}

void Texture::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
	// Resource can't be resized if it was never created in the first place.
	if (m_Resource)
	{
		CD3DX12_RESOURCE_DESC resDesc(m_Resource->GetDesc());

		resDesc.Width = std::max(width, 1u);
		resDesc.Height = std::max(height, 1u);
		resDesc.DepthOrArraySize = depthOrArraySize;
		resDesc.MipLevels = resDesc.MipLevels;

		auto device = m_Device.GetD3D12Device();

		auto p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&p,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COMMON,
			m_ClearValue.get(),
			IID_PPV_ARGS(&m_Resource)
		));

		// Retain the name of the resource if one was already specified.
		m_Resource->SetName(m_Name.c_str());

		ResourceStateTracker::AddGlobalResourceState(m_Resource.Get(), D3D12_RESOURCE_STATE_COMMON);
		CreateViews();
	}
}

//获取一个UAV的资源描述根据传入资源描述匹配
D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice = 0, UINT planeSlice = 0)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = resDesc.Format;

	switch (resDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		if (resDesc.DepthOrArraySize > 1)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
			uavDesc.Texture1DArray.MipSlice = mipSlice;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = mipSlice;
		}
		break;
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resDesc.DepthOrArraySize > 1)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
			uavDesc.Texture2DArray.PlaneSlice = planeSlice;
			uavDesc.Texture2DArray.MipSlice = mipSlice;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.PlaneSlice = planeSlice;
			uavDesc.Texture2D.MipSlice = mipSlice;
		}
		break;
	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
		uavDesc.Texture3D.FirstWSlice = arraySlice;
		uavDesc.Texture3D.MipSlice = mipSlice;
		break;
	default:
		throw std::exception("Invalid resource dimension.");
	}

	return uavDesc;
}

void Texture::CreateViews()
{
	if (m_Resource)
	{
		auto device = m_Device.GetD3D12Device();

		CD3DX12_RESOURCE_DESC desc(m_Resource->GetDesc());

		//创建RTV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport())
		{
			
			if (!m_IsCubeMap)
			{
				m_RenderTargetView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				
				device->CreateRenderTargetView(m_Resource.Get(), nullptr, m_RenderTargetView.GetDescriptorHandle());
			}
			else
			{
				//如果是纹理数组(例如CUBEMAP)
				//分别为数组的每一张纹理创建RTV
				m_RenderTargetView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_Resource->GetDesc().DepthOrArraySize * m_Resource->GetDesc().MipLevels);
				for (int i = 0; i < m_Resource->GetDesc().DepthOrArraySize; i++)
				{
					for (int mipSlice = 0; mipSlice < m_Resource->GetDesc().MipLevels; ++mipSlice)
					{
						//创建单独纹理的RTV描述
						D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
						rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						rtvDesc.Texture2DArray.MipSlice = mipSlice;
						rtvDesc.Texture2DArray.PlaneSlice = 0;
						rtvDesc.Format = m_Resource->GetDesc().Format;
						rtvDesc.Texture2DArray.FirstArraySlice = i;
						rtvDesc.Texture2DArray.ArraySize = 1;
						auto desc = m_Resource->GetDesc();
						device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_RenderTargetView.GetDescriptorHandle(i * m_Resource->GetDesc().MipLevels + mipSlice));
					}
				}
			}
		}
		//创建DSV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport())
		{


			if (!m_IsCubeMap)
			{
				m_DepthStencilView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
				device->CreateDepthStencilView(m_Resource.Get(), nullptr, m_DepthStencilView.GetDescriptorHandle());
			}
			else
			{
				//如果是纹理数组(例如CUBEMAP)
				//分别为数组的每一张纹理创建RTV
				m_DepthStencilView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_Resource->GetDesc().DepthOrArraySize * m_Resource->GetDesc().MipLevels);
				for (int i = 0; i < m_Resource->GetDesc().DepthOrArraySize; i++)
				{
					for (int mipSlice = 0; mipSlice < m_Resource->GetDesc().MipLevels; ++mipSlice)
					{
						//创建单独纹理的RTV描述
						D3D12_DEPTH_STENCIL_VIEW_DESC rtvDesc;
						rtvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						rtvDesc.Texture2DArray.MipSlice = mipSlice;
						rtvDesc.Format = m_Resource->GetDesc().Format;
						rtvDesc.Texture2DArray.FirstArraySlice = i;
						rtvDesc.Texture2DArray.ArraySize = 1;
						rtvDesc.Flags = D3D12_DSV_FLAG_NONE;
						auto desc = m_Resource->GetDesc();
						device->CreateDepthStencilView(m_Resource.Get(), &rtvDesc, m_DepthStencilView.GetDescriptorHandle(i * m_Resource->GetDesc().MipLevels + mipSlice));
					}
				}
			}
		}
		//创建SRV
			//创建SRV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && CheckSRVSupport())
		{
			if (!m_IsCubeMap)
			{
				m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				device->CreateShaderResourceView(m_Resource.Get(), nullptr, m_ShaderResourceView.GetDescriptorHandle());
			}
			else
			{
				auto cubemapDesc = m_Resource->GetDesc();
				cubemapDesc.Width = cubemapDesc.Height = 1024;
				cubemapDesc.DepthOrArraySize = 6;
				cubemapDesc.MipLevels = 0;

				D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSRVDesc = {};
				cubeMapSRVDesc.Format = cubemapDesc.Format;
				cubeMapSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				cubeMapSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				cubeMapSRVDesc.TextureCube.MipLevels = (UINT)-1;  // Use all mips.

				m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				device->CreateShaderResourceView(m_Resource.Get(), &cubeMapSRVDesc, m_ShaderResourceView.GetDescriptorHandle());

			}
		}

		//创建UAV
		if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 && CheckUAVSupport() && desc.DepthOrArraySize == 1)
		{
			m_UnorderAccessView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.MipLevels);
			for (int i = 0 ; i< desc.MipLevels;  i++)
			{
				auto uavDesc = GetUAVDesc(desc, i);
				device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, m_UnorderAccessView.GetDescriptorHandle(i));
			}
			
		}
	}
}

void Texture::CreateShaderResourceView()
{
	auto device = m_Device.GetD3D12Device();

	CD3DX12_RESOURCE_DESC desc(m_Resource->GetDesc());
	//创建SRV

	if (!m_IsCubeMap)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSRVDesc = {};
		cubeMapSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		cubeMapSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		cubeMapSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		cubeMapSRVDesc.TextureCube.MipLevels = (UINT)-1;  // Use all mips.

		m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		device->CreateShaderResourceView(m_Resource.Get(), &cubeMapSRVDesc, m_ShaderResourceView.GetDescriptorHandle());
	}
	else
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSRVDesc = {};
		cubeMapSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		cubeMapSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		cubeMapSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		cubeMapSRVDesc.TextureCube.MipLevels = (UINT)-1;  // Use all mips.

		m_ShaderResourceView = m_Device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		device->CreateShaderResourceView(m_Resource.Get(), &cubeMapSRVDesc, m_ShaderResourceView.GetDescriptorHandle());

	}

}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView() const
{
	return m_ShaderResourceView.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(uint32_t _mip) const
{
	return m_UnorderAccessView.GetDescriptorHandle(_mip);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRenderTargetView(UINT _index) const
{
	return m_RenderTargetView.GetDescriptorHandle(_index);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDepthStencilView(UINT _index) const
{
	return m_DepthStencilView.GetDescriptorHandle(_index);
}

bool Texture::HasAlpha() const
{
	DXGI_FORMAT format = GetResourceDesc().Format;

	bool hasAlpha = false;

	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		hasAlpha = true;
		break;
	}

	return hasAlpha;
}

size_t Texture::BitsPerPixel() const
{
	auto format = GetResourceDesc().Format;
	return DirectX::BitsPerPixel(format);
}

bool Texture::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SINT:
		return true;
	default:
		return false;
	}
}

bool Texture::IsSRGBFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

bool Texture::IsBGRFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return true;
	default:
		return false;
	}

}

bool Texture::IsDepthFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		return true;
	default:
		return false;
	}
}

DXGI_FORMAT Texture::GetTypelessFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT typelessFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
		typelessFormat = DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
		break;
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		typelessFormat = DXGI_FORMAT_R16_TYPELESS;
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		typelessFormat = DXGI_FORMAT_R8_TYPELESS;
		break;
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
		break;
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
		break;
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
		break;
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
		break;
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
		break;
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
		break;
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
		break;
	}

	return typelessFormat;
}

DXGI_FORMAT Texture::GetUAVCompatableFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT uavFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
		uavFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	}

	return uavFormat;
}

DXGI_FORMAT Texture::GetSRGBFormat(DXGI_FORMAT _format)
{
	DXGI_FORMAT srgbFormat = _format;
	switch (_format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		srgbFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC1_UNORM:
		srgbFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC2_UNORM:
		srgbFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC3_UNORM:
		srgbFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC7_UNORM:
		srgbFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}

	return srgbFormat;
}

