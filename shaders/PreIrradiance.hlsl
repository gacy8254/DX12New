struct PSInput
{
    float3 TexCoord : TEXCOORD;
};



TextureCube<float4> SkyboxTexture : register(t0);
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWarp : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);

float4 main(PSInput IN) : SV_TARGET
{
    const float PI = 3.14159265359;
    float3 normal = normalize(IN.TexCoord);
    
    float3 irradiance = (float3) 0.0f;
    
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 right = (cross(up, normal));
    up = (cross(normal, right));
    
    float sampleDelta = 0.025f;
    float nrSamples = 0.0f;
    
    for (float phi = 0.0f; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            
            irradiance += SkyboxTexture.Sample(gSamAnisotropicClamp, sampleVec).xyz * sin(theta) * cos(theta);
            nrSamples++;
        }
    }
    
    irradiance = PI * irradiance * (1.0f / nrSamples);
    
    return float4(irradiance, 1.0f);
}