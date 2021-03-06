#include "BasePSO.h"

BasePSO::BasePSO(std::shared_ptr<Device> _device)
	:m_Device(_device),
	m_DirtyFlags(DF_All),
	m_pPreviousCommandList(nullptr)
{
	m_pAlignedObjectCB = std::make_shared<ObjectCB>();
	m_pAlignedMainPassCB = std::make_shared<MainPass>();

//创建默认的空白SRV
D3D12_SHADER_RESOURCE_VIEW_DESC defaultSRV;
defaultSRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
defaultSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
defaultSRV.Texture2D.MostDetailedMip = 0;
defaultSRV.Texture2D.MipLevels = 1;
defaultSRV.Texture2D.PlaneSlice = 0;
defaultSRV.Texture2D.ResourceMinLODClamp = 0;
defaultSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

m_DefaultSRV = m_Device->CreateShaderResourceView(nullptr, &defaultSRV);

D3D12_SHADER_RESOURCE_VIEW_DESC defaultSRVCube;
defaultSRVCube.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
defaultSRVCube.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
defaultSRVCube.TextureCube.MipLevels = 1;
defaultSRVCube.TextureCube.MostDetailedMip = 0;
defaultSRVCube.TextureCube.ResourceMinLODClamp = 0;
defaultSRVCube.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

m_DefaultSRVCube = m_Device->CreateShaderResourceView(nullptr, &defaultSRVCube);
}

BasePSO::~BasePSO()
{
	//_aligned_free(m_pAlignedObjectCB);
	//_aligned_free(m_pAlignedMainPassCB);
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 8> BasePSO::GetStaticSamplers()
{
	//过滤器POINT,寻址模式WRAP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC pointWarp(0,	//着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//过滤器类型为POINT(常量插值)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

	//过滤器POINT,寻址模式CLAMP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,	//着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//过滤器类型为POINT(常量插值)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

	//过滤器LINEAR,寻址模式WRAP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC linearWarp(2,	//着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//过滤器类型为LINEAR(线性插值)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

	//过滤器LINEAR,寻址模式CLAMP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,	//着色器寄存器
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//过滤器类型为LINEAR(线性插值)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

	//过滤器ANISOTROPIC,寻址模式WRAP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,	//着色器寄存器
		D3D12_FILTER_ANISOTROPIC,			//过滤器类型为ANISOTROPIC(各向异性)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

	//过滤器LINEAR,寻址模式CLAMP的静态采样器
	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,	//着色器寄存器
		D3D12_FILTER_ANISOTROPIC,			//过滤器类型为ANISOTROPIC(各向异性)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

	CD3DX12_STATIC_SAMPLER_DESC shadowSample(6,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	CD3DX12_STATIC_SAMPLER_DESC shadowSample2(7,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
#if USE_REVERSE_Z
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,
#else
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
#endif
	D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
		

	return{ pointWarp, pointClamp, linearWarp, linearClamp, anisotropicWarp, anisotropicClamp, shadowSample, shadowSample2 };
}

void BasePSO::BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture, UINT _slot, bool _cubeMap)
{
	//如果贴图有效就设置贴图,否则就用默认的SRV填充
	if (_texture)
	{
		_commandList.SetShaderResourceView(_slot, _offset, _texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		if (_cubeMap)
		{
			_commandList.SetShaderResourceView(_slot, _offset, m_DefaultSRVCube, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
		else
		{
			_commandList.SetShaderResourceView(_slot, _offset, m_DefaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
		
	}
}
