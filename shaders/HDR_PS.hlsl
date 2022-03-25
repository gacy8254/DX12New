#include "CommonTool.hlsli"

struct PixelShaderInput
{
    float4 PositionVS : POSITION; //观察空间坐标
    float3 NormalVS : NORMAL; //观察空间发现
    float2 TexCoord : TEXCOORD; //UV
};

ConstantBuffer<Material> MaterialCB : register(b0, space1);
ConstantBuffer<LightProperties> LightPropertiesCB : register(b1);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
Texture2D DiffuseTexture : register(t2);

SamplerState LinerRepeatSampler : register(s0);

LightResult DoLighting(float3 _pos, float3 _normal)
{
    uint i;
    
    float3 V = normalize(-_pos);
    
    LightResult totalResult;
    
    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        LightResult result = DoPointLight(PointLights[i], V, _pos, _normal, MaterialCB.SpecularPower);

        totalResult.Specular += result.Specular;
        totalResult.Diffuse += result.Diffuse;
    }
    
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        LightResult result = DoSpotLight(SpotLights[i], V, _pos, _normal, MaterialCB.SpecularPower);

        totalResult.Specular += result.Specular;
        totalResult.Diffuse += result.Diffuse;
    }

    return totalResult;
}

float4 main(PixelShaderInput IN) : SV_Target
{
    LightResult lit = DoLighting(IN.PositionVS.xyz, normalize(IN.NormalVS));
    
    float4 emissive = MaterialCB.Emissive;
    float4 ambient = MaterialCB.Ambient;
    float4 diffuse = MaterialCB.Diffuse * lit.Diffuse;
    float4 specular = MaterialCB.Specular * lit.Specular;
    float4 texColor = DiffuseTexture.Sample(LinerRepeatSampler, IN.TexCoord);
    
    return (emissive + ambient + diffuse + specular) * texColor;
}
