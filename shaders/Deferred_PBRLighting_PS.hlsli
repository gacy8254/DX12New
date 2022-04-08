#include "PBR_Function.hlsli"
#include "MainPassCB.hlsli"

#define USE_CBDR 1

#define DEBUG 1

//深度分片划分
static const float DepthSlicing_16[17] =
{
    1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};

struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

struct CameraPropertices
{
    float4 CameraPos;
};

ConstantBuffer<CameraPropertices> CameraProperticesCB : register(b2);
// Textures
Texture2D AlbedoText                : register(t4);
Texture2D NormalText                : register(t5);
Texture2D ORMText                   : register(t6);
Texture2D EmissiveText              : register(t7);
Texture2D WorldPosText              : register(t8);
TextureCube<float4> IrradianceText  : register(t9);
TextureCube<float4> PrefilterText   : register(t10);
Texture2D IntegrateBRDFText         : register(t11);
Texture2D DepthText                 : register(t12);

SamplerState SamPointWrap           : register(s0);
SamplerState SamPointClamp          : register(s1);
SamplerState SamLinearWarp          : register(s2);
SamplerState SamLinearClamp         : register(s3);
SamplerState SamAnisotropicWarp     : register(s4);
SamplerState SamAnisotropicClamp    : register(s5);

float LinearDepth(float depth)
{
    return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float ViewDepth(float depth)
{
    return (FAR_Z * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

//计算所有灯光的直接光照
float3 DoLighting(float2 _uv, float3 _normal, float3 _worldPos, float3 _toCamera, float3 _albedo, float _roughness, float _metallic, float _shadowAmount = 1.0f)
{
    float3 totalResult = (float3) 0.0f;
    int i = 0;
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, _albedo.rgb, _metallic);
    
    //深度
    float depthBuffer = DepthText.Sample(SamAnisotropicWarp, _uv).r;
    float depth = ViewDepth(depthBuffer);
    depth = depthBuffer;
    float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);
    
    if (linearDepth <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    uint gridID = 0;
#if USE_CBDR
    //计算像素所在分簇的Z轴是第几个
    uint clusterZ = 0;
    for (clusterZ = 0; ((depth > DepthSlicing_16[clusterZ + 1]) && (clusterZ < CLUSTER_NUM_Z - 1)); clusterZ++)
    {
    }
    
    uint offsetX = floor(_uv.x * gRTSize.x / CLUSTER_SIZE_X);
    uint offsety = floor(_uv.y * gRTSize.y / CLUSTER_SIZE_Y);
    gridID = (offsety * ceil(gRTSize.x / CLUSTER_SIZE_X) + offsetX) * CLUSTER_NUM_Z + clusterZ;
    
    uint numPointLight = LightsList[gridID].NumPointLights;
    uint numSpotLight = LightsList[gridID].NumSpotLights;
    if (numPointLight > MAX_GRID_POINT_LIGHT_NUM)
    {
        numPointLight = MAX_GRID_POINT_LIGHT_NUM;
    }
    if (numSpotLight > MAX_GRID_SPOTLIGHT_NUM)
    {
        numSpotLight = MAX_GRID_SPOTLIGHT_NUM;
    }
#endif
    
#if USE_CBDR
    
    for (i = 0; i < numPointLight; ++i)
    {
        float3 result = DoPointLight(PointLights[LightsList[gridID].PointlightIndices[i]], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }

    // Iterate spot lights.
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        float3 result = DoSpotLight(SpotLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }

    // Iterate directinal lights
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        float3 result = DoDirectionalLight(DirectionalLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }
#else
    
    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        float3 result = DoPointLight(PointLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }

    // Iterate spot lights.
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        float3 result = DoSpotLight(SpotLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }

    // Iterate directinal lights
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        float3 result = DoDirectionalLight(DirectionalLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);

        totalResult += result;
    }
#endif  
   //
#if DEBUG
    float lightNum = float(numPointLight + 0) / 1.0f;
	{

        float4 color = float4(lightNum, lightNum, lightNum, 1.0f);
        return color;
    }
#else
    return totalResult;
#endif
}

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
    float3 directColor;
#if ENABLE_LIGHTING
    //计算直接光照
    directColor = DoLighting(IN.TexCoord.xy, normal.xyz, WorldPos, toCamera, albedo.rgb, roughness, metallic);
    
    //计算简介光照
    float3 ambientDiffuse;
    float3 ambientSpecular;
    AmbientPBR(normal.rgb, WorldPos, toCamera, albedo.rgb, roughness, metallic, irradiance, prefilterColor, brdf, 1.0f, float2(ao, ao), ambientDiffuse, ambientSpecular);
    
    //最终光照结果
    color = directColor + ambientSpecular + ambientDiffuse;
#else 
  //shadow = -N.z;
#endif // ENABLE_LIGHTING
   
#if DEBUG
    float3 ccc = (float3) (directColor);
    return float4(ccc, alpha);
#else
    return float4(color, alpha);
#endif
}

