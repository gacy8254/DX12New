struct PSInput
{
    float3 TexCoord : TEXCOORD;
    
};

TextureCube<float4> SkyboxTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

float4 main(PSInput IN) : SV_TARGET
{
    return SkyboxTexture.Sample(LinearClampSampler, IN.TexCoord);
}