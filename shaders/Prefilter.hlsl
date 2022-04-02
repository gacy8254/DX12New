#include "PBR_Function.hlsli"

struct PSInput
{
    float3 TexCoord : TEXCOORD;
};

cbuffer cBuffer : register(b1)
{
    float roughness;
}

TextureCube<float4> SkyboxTexture : register(t0);
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWarp : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);

float4 main(PSInput IN) : SV_TARGET
{
    float3 normal = normalize(IN.TexCoord);
    float3 reflectVec = normal;
    float3 v = reflectVec;
    
    
    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0f;
    float3 prefilterColor = (float3) 0.0f;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(xi, normal, roughness);
        float3 L = normalize(2.0f * dot(v, H) * H - v);
        
        float NdotL = max(dot(normal, L), 0.0f);
        if(NdotL > 0.0f)
        {
            float D = DistributionGGX(normal, H, roughness);
            float NdotH = max(dot(normal, H), 0.0);
            float HdotV = max(dot(H, v), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
            
            float resolution = 1024.0f;
            float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
            float saSample = 1.0 / ((float) SAMPLE_COUNT * pdf + 0.0001);
            
            float mipLevel = roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
            
            
            prefilterColor += SkyboxTexture.SampleLevel(gSamLinearClamp, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;

        }

    }
    
    prefilterColor = prefilterColor / totalWeight;
    
    float gamma = 2.2;
    
    float3 finalColor;
    
    finalColor = pow(prefilterColor, (float3) (gamma));
    
    return float4(finalColor, 1.0f);
}