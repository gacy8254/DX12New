#include "PBR_Function.hlsli"

struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

struct CameraPropertices
{
    float4 CameraPos;
};

ConstantBuffer<CameraPropertices> CameraProperticesCB : register(b1);

// Textures
Texture2D AlbedoText : register(t3);
Texture2D NormalText : register(t4);
Texture2D ORMText : register(t5);
Texture2D EmissiveText : register(t6);
Texture2D WorldPosText : register(t7);
TextureCube<float4> IrradianceText : register(t8);
TextureCube<float4> PrefilterText : register(t9);
Texture2D IntegrateBRDFText : register(t10);

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWarp : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWarp : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

float4 main(PixelShaderInput IN) : SV_Target
{
    //从贴图中获取数据
    float3 WorldPos = WorldPosText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float4 albedo = AlbedoText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float ao = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).r;
    float roughness = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).g;
    float metallic = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).b;
    float4 normal = NormalText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float4 emissive = EmissiveText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float alpha = albedo.a;
    
    //像素到相机的向量
    float3 toCamera = normalize(CameraProperticesCB.CameraPos.xyz - WorldPos);
    
    //反射向量
    float3 reflectVec = reflect(-toCamera, normal.rgb);
    
    //漫反射环境光
    float3 irradiance = IrradianceText.Sample(SamAnisotropicWarp, normal.rgb).rgb;
    
    //镜面反射环境光
    const float MAX_REFLECTION_LOD = 4.0F;
    float3 prefilterColor = PrefilterText.SampleLevel(SamAnisotropicClamp, reflectVec, roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = IntegrateBRDFText.Sample(SamAnisotropicClamp, float2(max(dot(normal.rgb, toCamera), 0.0f), roughness)).rg;
    
    float3 color = (float3) (albedo.rgb);
    float shadow = 1;
#if ENABLE_LIGHTING
    //计算直接光照
    float3 directColor = DoLighting(normal.xyz, WorldPos, toCamera, albedo.rgb, roughness, metallic);
    
    //计算简介光照
    float3 ambientDiffuse;
    float3 ambientSpecular;
    AmbientPBR(normal.rgb, WorldPos, toCamera, albedo.rgb, roughness, metallic, irradiance, prefilterColor, brdf, 1.0f, float2(ao, ao), ambientDiffuse, ambientSpecular);
    
    //最终光照结果
    color = directColor + ambientSpecular + ambientDiffuse;
#else 
  //shadow = -N.z;
#endif // ENABLE_LIGHTING

    return float4(color, alpha);
    float3 ccc = (float3) (color.rgg);
    //return float4(ccc, alpha);
}

