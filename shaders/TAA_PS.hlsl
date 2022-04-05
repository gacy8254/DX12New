#include "MainPassCB.hlsli"

struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

Texture2D InputTexture : register(t0);
Texture2D HistoryTexture : register(t1);

struct PixelOutput
{
    float4 current : SV_TARGET0;
    float4 history : SV_TARGET1;
};

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWarp : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWarp : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

PixelOutput main(PixelShaderInput IN)
{

    PixelOutput output;
    float4 currentColor = InputTexture.Sample(SamAnisotropicClamp, IN.TexCoord);
    float4 historytColor = HistoryTexture.Sample(SamAnisotropicClamp, IN.TexCoord);
    
    if(gFrameCount == 0)
    {
        output.current = currentColor;
        output.history = currentColor;
        
        return output;
    }

    output.current = currentColor * 0.1 + (1 - 0.1) * historytColor;
    output.history = currentColor * 0.1 + (1 - 0.1) * historytColor;
    
    
    
	return output;
}