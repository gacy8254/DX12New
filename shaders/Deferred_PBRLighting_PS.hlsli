#include "PBR_Function.hlsli"
#include "MainPassCB.hlsli"

#define USE_CBDR 1

#define DEBUG 0

//深度分片划分
static const float DepthSlicing_16[17] =
{
    1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};

static const float DepthSlicing_64[65] =
{
    1, 2, 3, 5, 8, 12, 17, 23, 30, 38, 47, 57, 68, 70, 83, 97,
    117, 147, 187, 237, 297, 367, 447, 537, 637, 747, 867, 997, 1137, 1287, 1447, 1617,
    1778, 1965, 2152, 2367, 2604, 2864, 3151, 3466, 3812, 4194, 4613, 5074, 5582, 6140, 6754, 7430,
    8544, 9826, 11300, 12995, 14944, 17186, 19764, 22728, 26138, 30058, 34567, 39752, 45715, 46715, 47715, 48715,
    50000.0f
};

static const float3 sampleOffsetDirections[20] =
{
    float3(1, 1, 1), float3(1, -1, 1), float3(-1, -1, 1), float3(-1, 1, 1),
   float3(1, 1, -1), float3(1, -1, -1), float3(-1, -1, -1), float3(-1, 1, -1),
   float3(1, 1, 0), float3(1, -1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
   float3(1, 0, 1), float3(-1, 0, 1), float3(1, 0, -1), float3(-1, 0, -1),
   float3(0, 1, 1), float3(0, -1, 1), float3(0, -1, -1), float3(0, 1, -1)
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
Texture2D AlbedoText                        : register(t4);
Texture2D NormalText                        : register(t5);
Texture2D ORMText                           : register(t6);
Texture2D EmissiveText                      : register(t7);
Texture2D WorldPosText                      : register(t8);
TextureCube<float4> IrradianceText          : register(t9);
TextureCube<float4> PrefilterText           : register(t10);
Texture2D IntegrateBRDFText                 : register(t11);
Texture2D DepthText                         : register(t12);
TextureCube<float4> ShadowMapText[10]       : register(t13);

SamplerState SamPointWrap                   : register(s0);
SamplerState SamPointClamp                  : register(s1);
SamplerState SamLinearWarp                  : register(s2);
SamplerState SamLinearClamp                 : register(s3);
SamplerState SamAnisotropicWarp             : register(s4);
SamplerState SamAnisotropicClamp            : register(s5);

float LinearDepth(float depth)
{
    return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float ViewDepth(float depth)
{
    return (FAR_Z * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

float CalcuShadow(float _bias, float _closetDepth, float _currentdepth)
{   
    if (_closetDepth > _currentdepth - _bias)
    {
        return 1;
    }
    else
    {
        return 0;
    }
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
    //depth = depthBuffer;
    float linearDepth = (depth - NEAR_Z) / (FAR_Z - NEAR_Z);
    
    if (linearDepth <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    uint gridID = 0;
#if USE_CBDR
    //计算像素所在分簇的Z轴是第几个
    uint clusterZ = 1;
    for (clusterZ = 0; ((depth > DepthSlicing_64[clusterZ + 1]) && (clusterZ < CLUSTER_NUM_Z - 1)); clusterZ++)
    {
        ;
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
    [unroll(200)]
    for (i = 0; i < numPointLight; ++i)
    {
        float3 result = DoPointLight(PointLights[LightsList[gridID].PointlightIndices[i]], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);
    
#if CAST_SHADOW
        float shadow = 0.0f;

        //获取点到光源的向量
        float3 toLight = _worldPos - PointLights[i].PositionWS.xyz;
        //点到光源的距离
        float currentDepth = length(toLight);
        
        //偏移
        float bias = max(0.05 * (1.0f - dot(_normal, -toLight)), 0.005f);
        
        //PCF
        float samples = 20.0f;
        float disToView = length(_toCamera);
        float diskRadius = (1.0f + (disToView / 1000.0f)) / 300.0f;

        for (float j = 0; j < samples; j++)
        {
            //从贴图中采样最近点的距离,使用偏移来进行模糊
            float closestdepth = ShadowMapText[i].Sample(SamAnisotropicClamp, toLight + sampleOffsetDirections[j] * diskRadius).r;
            //判断是否处于阴影中
            shadow += CalcuShadow(bias, closestdepth, currentDepth);
        }
        shadow /= samples;

        //光照结果乘阴影值
        result *= (float3)shadow;
#endif

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
        //计算光照结果
        float3 result = DoPointLight(PointLights[i], _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, F0);
        
#if CAST_SHADOW
        float shadow = 0.0f;

        //获取点到光源的向量
        float3 toLight = _worldPos - PointLights[i].PositionWS.xyz;
        //点到光源的距离
        float currentDepth = length(toLight);
        
        //偏移
        float bias = max(0.05 * (1.0f - dot(_normal, -toLight)), 0.005f);
        
        //PCF
        float samples = 20.0f;
        float disToView = length(_toCamera);
        float diskRadius = (1.0f + (disToView / 1000.0f)) / 300.0f;

        for (float j = 0; j < samples; j++)
        {
            //从贴图中采样最近点的距离,使用偏移来进行模糊
            float closestdepth = ShadowMapText[i].Sample(SamAnisotropicClamp, toLight + sampleOffsetDirections[j] * diskRadius).r;
            //判断是否处于阴影中
            shadow += CalcuShadow(bias, closestdepth, currentDepth);
        }
        shadow /= samples;

        //光照结果乘阴影值
        result *= (float3)shadow;
#endif
        
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
    float clusterColor = float(clusterZ) / CLUSTER_NUM_Z;
    float lightNum = float(numPointLight + 0) / 200.0f;
	{
        float4 color;
        if (_uv.x * gRTSize.x  < gRTSize.x / 2.0f)
        {
            color = float4(clusterColor, clusterColor, clusterColor, 1.0f);
        }
        else
        {
            color = float4(lightNum, lightNum, lightNum, 1.0f);
        }
        
        return color;
    }
 //   return totalResult;
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
    float3 toLight = WorldPos - PointLights[0].PositionWS.xyz;
    float3 pos = ShadowMapText[1].Sample(SamAnisotropicClamp, toLight).rgb;
    float3 aaaaa = ShadowMapText[0].Sample(SamAnisotropicClamp, WorldPos).rgb;
    float3 ccc = (float3) (directColor);
    if (IN.TexCoord.x * gRTSize.x < gRTSize.x / 2.0f)
    {
        ccc = WorldPos;
       
    }
    else
    {
        ccc = pos;
    }
    
    
    return float4(ccc, alpha);
#else
    return float4(color, alpha);
#endif
}

