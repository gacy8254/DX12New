#pragma once
#include "CommandList.h"
#include "Device.h"
#include "helpers.h"
#include "Material.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include "ShaderDefinition.h"
#include <array>

class BasePSO
{
public:
	struct alignas(16) Matrices
	{
		Matrix4 ModelMatrix;
		Matrix4 ModelViewMatrix;
		Matrix4 InverseTransposeModelMatrix;
		Matrix4 ModelViewProjectionMatrix;
	};

	BasePSO(std::shared_ptr<Device> _device);
	virtual ~BasePSO();

	const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
	void SetMaterial(const std::shared_ptr<Material>& _material)
	{
		m_Material = _material;
		m_DirtyFlags |= DF_Material;
	}

	void XM_CALLCONV SetWorldMatrix(Matrix4 worldMatrix)
	{
		m_pAlignedMVP->World = worldMatrix;
		m_DirtyFlags |= DF_Matrices;
	}
	Matrix4 GetWorldMatrix() const { return m_pAlignedMVP->World; }

	void XM_CALLCONV SetViewMatrix(Matrix4 viewMatrix)
	{
		m_pAlignedMVP->View = viewMatrix;
		m_DirtyFlags |= DF_Matrices;
	}
	Matrix4 GetViewMatrix() const { return m_pAlignedMVP->View; }

	void XM_CALLCONV SetProjectionMatrix(Matrix4 projectionMatrix)
	{
		m_pAlignedMVP->Projection = projectionMatrix;
		m_DirtyFlags |= DF_Matrices;
	}
	Matrix4 GetProjectionMatrix() const { return m_pAlignedMVP->Projection; }

	void XM_CALLCONV SetCameraPos(Vector4 _pos)
	{
		m_pAlignedMVP->CamPos = _pos;
		m_DirtyFlags |= DF_Matrices;
	}
	Vector4 GetCameraPos() const { return m_pAlignedMVP->CamPos; }

	//应用到渲染管线上
	virtual void Apply(CommandList& _commandList) = 0;

	static std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

protected:
	enum DirtyFlags
	{
		DF_None = 0,
		DF_PointLights = (1 << 0),
		DF_SpotLights = (1 << 1),
		DF_DirectionalLights = (1 << 2),
		DF_Material = (1 << 3),
		DF_Matrices = (1 << 4),
		DF_All = DF_PointLights | DF_SpotLights | DF_DirectionalLights | DF_Material | DF_Matrices
	};

	struct alignas(16) MVP
	{
		Matrix4 World;
		Matrix4 View;
		Matrix4 Projection;
		Vector4 CamPos;
	};

	void BindTexture(CommandList& _commandList, uint32_t _offset, const std::shared_ptr<Texture>& _texture, UINT _slot);

	std::shared_ptr<Device> m_Device;
	std::shared_ptr<RootSignature> m_RootSignature;
	std::shared_ptr<PipelineStateObject> m_PSO;

	std::shared_ptr<Material> m_Material;

	//默认的空白SRV用于占位贴图槽
	std::shared_ptr<ShaderResourceView> m_DefaultSRV;

	MVP* m_pAlignedMVP;

	CommandList* m_pPreviousCommandList;

	//有哪些需要绑定的属性
	uint32_t m_DirtyFlags;

};

