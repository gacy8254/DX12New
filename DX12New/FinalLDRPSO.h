#pragma once
#include "BasePSO.h"
#include "Texture.h"
#include "CommandList.h"

#include <memory>
#include <vector>

class FinalLDRPSO :
    public BasePSO
{
public:
	enum TonemapMethod : uint32_t
	{
		TM_Linear,
		TM_Reinhard,
		TM_ReinhardSq,
		TM_ACESFilmic,
	};

	struct Tonemap
	{
		Tonemap()
			: TonemapMethod(TM_Reinhard)
			, Exposure(0.0f)
			, MaxLuminance(1.0f)
			, K(1.0f)
			, A(0.22f)
			, B(0.3f)
			, C(0.1f)
			, D(0.2f)
			, E(0.01f)
			, F(0.3f)
			, LinearWhite(11.2f)
			, Gamma(2.2f)
		{}

		// The method to use to perform tonemapping.
		TonemapMethod TonemapMethod;
		// Exposure should be expressed as a relative exposure value (-2, -1, 0, +1, +2 )
		float Exposure;

		// The maximum luminance to use for linear tonemapping.
		float MaxLuminance;

		// Reinhard constant. Generally this is 1.0.
		float K;

		// ACES Filmic parameters
		// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
		float A;  // Shoulder strength
		float B;  // Linear strength
		float C;  // Linear angle
		float D;  // Toe strength
		float E;  // Toe Numerator
		float F;  // Toe denominator
		// Note E/F = Toe angle.
		float LinearWhite;
		float Gamma;
	};

	enum RootParameters
	{
		TonemapParameters,	//					: register(b0);
		Textures,  // Texture2D AmbientTexture       : register( t0 );

				   NumRootParameters
	};

	FinalLDRPSO(std::shared_ptr<Device> _device);
	virtual ~FinalLDRPSO();

	void SetTonemapParameters(Tonemap _tonemapParameters) 
	{ 
		m_TonemapParameters = _tonemapParameters; 
		m_DirtyFlags |= DF_ObjectCB;
	}
	Tonemap GetTonemapParameters() const { return m_TonemapParameters; }

	void SetTexture(std::shared_ptr<Texture> _textures)
	{
		m_Texture = _textures;
		m_DirtyFlags |= DF_Material;
	}

	//应用到渲染管线上
	void Apply(CommandList& _commandList) override;

private:
	std::shared_ptr<Texture> m_Texture;

	//映射参数和方法
	Tonemap m_TonemapParameters;
};

