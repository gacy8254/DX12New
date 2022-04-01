#include "BasePSO.h"

BasePSO::BasePSO(std::shared_ptr<Device> _device)
	:m_Device(_device),
	m_DirtyFlags(DF_All),
	m_pPreviousCommandList(nullptr)
{
	m_pAlignedMVP = (MVP*)_aligned_malloc(sizeof(MVP), 16);

//����Ĭ�ϵĿհ�SRV
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

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> BasePSO::GetStaticSamplers()
{
	//������POINT,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC pointWarp(0,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//����������ΪPOINT(������ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������POINT,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//����������ΪPOINT(������ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	//������LINEAR,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC linearWarp(2,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//����������ΪLINEAR(���Բ�ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������LINEAR,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//����������ΪLINEAR(���Բ�ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	//������ANISOTROPIC,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,	//��ɫ���Ĵ���
		D3D12_FILTER_ANISOTROPIC,			//����������ΪANISOTROPIC(��������)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������LINEAR,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,	//��ɫ���Ĵ���
		D3D12_FILTER_ANISOTROPIC,			//����������ΪANISOTROPIC(��������)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	return{ pointWarp, pointClamp, linearWarp, linearClamp, anisotropicWarp, anisotropicClamp };
}

void BasePSO::BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture, UINT _slot)
{
	//�����ͼ��Ч��������ͼ,�������Ĭ�ϵ�SRV���
	if (_texture)
	{
		_commandList.SetShaderResourceView(_slot, _offset, _texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	else
	{
		_commandList.SetShaderResourceView(_slot, _offset, m_DefaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}
