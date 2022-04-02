#include "PBR_Function.hlsli"

// clang-format off
struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

float2 main(PixelShaderInput IN) : SV_TARGET
{
    float2 brdf = IntegrateBRDF(IN.TexCoord.x, IN.TexCoord.y);
    return brdf;
}