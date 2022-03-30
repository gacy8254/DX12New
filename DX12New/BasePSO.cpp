#include "BasePSO.h"

BasePSO::BasePSO(std::shared_ptr<Device> _device)
	:m_Device(_device),
	m_DirtyFlags(DF_All),
	m_pPreviousCommandList(nullptr)
{
	m_pAlignedMVP = (MVP*)_aligned_malloc(sizeof(MVP), 16);

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
}

BasePSO::~BasePSO()
{
	_aligned_free(m_pAlignedMVP);
}

void BasePSO::BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture, UINT _slot)
{
	//如果贴图有效就设置贴图,否则就用默认的SRV填充
	if (_texture)
	{
		_commandList.SetShaderResourceView(_slot, _offset, _texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		_commandList.SetShaderResourceView(_slot, _offset, m_DefaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}
