struct PSInput
{
    float3 TexCoord : TEXCOORD;
};

TextureCube<float4> SkyboxTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

float4 main(PSInput IN) : SV_TARGET
{
    
    float gamma = 2.0;
    
    float3 finalColor = SkyboxTexture.Sample(LinearClampSampler, IN.TexCoord).rgb;
    
    finalColor = pow(finalColor, (float3) (gamma));
    
    return float4(finalColor, 1.0f);
}