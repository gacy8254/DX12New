#include "PBR_Function.hlsli"

// clang-format off
struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

//#if ENABLE_LIGHTING
struct PointLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float  Ambient;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    float  QuadraticAttenuation;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct SpotLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionWS; // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS; // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float  Ambient;
    float  SpotAngle;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    //----------------------------------- (16 byte boundary)
    float  QuadraticAttenuation;
    float3 Padding;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 7 = 112 bytes
};

struct DirectionalLight
{
    float4 DirectionWS;  // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS;  // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float Ambient;
    float3 Padding;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct LightProperties
{
    uint NumPointLights;
    uint NumSpotLights;
    uint NumDirectionalLights;
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
    float4 Ambient;
};

struct CameraPropertices
{
    float4 CameraPos;
};

ConstantBuffer<LightProperties> LightPropertiesCB : register( b0 );
ConstantBuffer<CameraPropertices> CameraProperticesCB : register(b1);

StructuredBuffer<PointLight> PointLights : register( t0 );
StructuredBuffer<SpotLight> SpotLights : register( t1 );
StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 );
//#endif // ENABLE_LIGHTING

// Textures
Texture2D AlbedoText : register(t3);
Texture2D NormalText : register(t4);
Texture2D ORMText : register(t5);
Texture2D EmissiveText : register(t6);
Texture2D WorldPosText : register(t7);

SamplerState TextureSampler : register(s0);

//#if ENABLE_LIGHTING
float DoDiffuse( float3 N, float3 L )
{
    return max( 0, dot( N, L ) );
}

float DoSpecular( float3 V, float3 N, float3 L, float specularPower )
{
    float3 R = normalize( reflect( -L, N ) );
    float RdotV = max( 0, dot( R, V ) );

    return pow( RdotV, specularPower );
}

float DoAttenuation(float d )
{
    return 1.0f / (d * d );
}

float DoSpotCone( float3 spotDir, float3 L, float spotAngle )
{
    float minCos = cos( spotAngle );
    float maxCos = ( minCos + 1.0f ) / 2.0f;
    float cosAngle = dot( spotDir, -L );
    return smoothstep( minCos, maxCos, cosAngle );
}

LightResult DoPointLight( PointLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness )
{
    LightResult result;
    float3 toCamera = normalize(CameraProperticesCB.CameraPos.xyz - P);
    
    float3 toLight = normalize(light.PositionWS.xyz - P);
    float3 halfVec = normalize(toCamera + toLight);
    
    float dis = length(light.PositionWS.xyz - P);
    float attenuation = DoAttenuation(dis);
    
    float3 radiance = light.Color * attenuation;
    
    float NDF = DistributionGGX(N.xyz, halfVec, _roughness);
    float G = GeometrySmith(N.xyz, toCamera, toLight, _roughness);
    
    float3 F = CalFresnel(halfVec, toCamera, _albedo.xyz, _metallic);
    
    float3 numerator = NDF * G * F;
    float demoninator = 4.0 * max(dot(N.xyz, toCamera), 0.0f) * max(dot(N.xyz, toLight), 0.0) + 0.001;
    
    float3 specular = numerator / demoninator;
    
    float3 KS = F;
    
    float3 kd = (float3) 1.0f - KS;
    
    kd *= 1.0f - _metallic;
    
    float NdotL = max(dot(N.xyz, toLight), 0.0);
    
    float3 lo = (kd * _albedo.xyz / PI + specular) * radiance * NdotL;

    
    result.Diffuse.rgb = (float3) lo;
    result.Specular.rgb = lo;
    result.Ambient.rgb = lo;

    return result;
}

LightResult DoSpotLight(SpotLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness)
{
    LightResult result;
    
    float3 toLight = (light.PositionVS.xyz - P);
    float3 halfVec = normalize(toLight + V);
    
    float d = length(toLight);
    toLight = normalize(-light.DirectionWS);

    float attenuation = DoAttenuation(d );
    float spotIntensity = DoSpotCone(light.DirectionVS.xyz, toLight, light.SpotAngle);

    float3 radiance = light.Color * attenuation * spotIntensity;
    
    float f = CalFresnel(halfVec, V, _albedo, _metallic);
    float NDF = DistributionGGX(N, halfVec, _roughness);
    float G = GeometrySmith(N, V, toLight, _roughness);
    
    float3 nominator = NDF * G * f;
    float denominator = 4.0 * max(dot(N, V), 0.0f) * max(dot(N, toLight), 0.0f) + 0.001f;
    
    float3 specular = nominator / denominator;
    
    float3 KS = f;
    float3 KD = float3(1.0f, 1.0f, 1.0f) - KS;
    
    KD *= 1.0f - _metallic;
    
    float NdotL = max(dot(N, toLight), 0.0f);
    
    float3 Lo = (KD * _albedo / PI + specular) * radiance * NdotL;
    
    result.Diffuse.rgb = Lo;
    result.Specular.rgb = Lo;
    result.Ambient.rgb = Lo;

    return result;
}

LightResult DoDirectionalLight(DirectionalLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness)
{
    LightResult result = (LightResult)0;

    float3 toLight = normalize(-light.DirectionWS.xyz);
    float3 halfVec = normalize(toLight + V);

    float3 radiance = light.Color;
    
    float f = CalFresnel(halfVec, V, _albedo, _metallic);
    float NDF = DistributionGGX(N, halfVec, _roughness);
    float G = GeometrySmith(N, V, toLight, _roughness);
    
    float3 nominator = NDF * G * f;
    float denominator = 4.0 * max(dot(N, V), 0.0f) * max(dot(N, toLight), 0.0f) + 0.001f;
    
    float3 specular = nominator / denominator;
    
    float3 KS = f;
    float3 KD = float3(1.0f, 1.0f, 1.0f) - KS;
    
    KD *= 1.0f - _metallic;
    
    float NdotL = max(dot(N, toLight), 0.0f);
    
    float3 Lo = (KD * _albedo / PI + specular) * radiance * NdotL;
    
    //result.Diffuse.rgb = (float3) Lo;
    result.Diffuse.rgb = Lo;
    result.Specular.rgb = Lo;
    result.Ambient.rgb = Lo;

    return result;
}

LightResult DoLighting(float3 P, float3 N,  float3 _camPos, float3 _albedo, float _metallic, float _roughness)
{
    uint i;

    // Lighting is performed in view space.
    float3 V = normalize(_camPos - P);

    LightResult totalResult = (LightResult)0;

    // Iterate point lights.
    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        LightResult result = DoPointLight(PointLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    // Iterate spot lights.
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        LightResult result = DoSpotLight(SpotLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    // Iterate directinal lights
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        LightResult result = DoDirectionalLight(DirectionalLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    totalResult.Diffuse = saturate( totalResult.Diffuse );
    totalResult.Specular = saturate( totalResult.Specular );
    totalResult.Ambient = saturate( totalResult.Ambient );

    return totalResult;
}
//#endif // ENABLE_LIGHTING

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 WorldPos = WorldPosText.Sample(TextureSampler, IN.TexCoord.xy);
    float4 albedo = AlbedoText.Sample(TextureSampler, IN.TexCoord.xy);
    float ao = ORMText.Sample(TextureSampler, IN.TexCoord.xy).r;
    float roughness = ORMText.Sample(TextureSampler, IN.TexCoord.xy).g;
    float metallic = ORMText.Sample(TextureSampler, IN.TexCoord.xy).b;
    float4 normal = NormalText.Sample(TextureSampler, IN.TexCoord.xy);
    float4 emissive = EmissiveText.Sample(TextureSampler, IN.TexCoord.xy);
    float alpha = albedo.a;
    
    float3 diffuse = albedo;
    float shadow = 1;
    float4 specular = 0;
//#if ENABLE_LIGHTING
    LightResult lit = DoLighting(WorldPos, normal.rgb, CameraProperticesCB.CameraPos.xyz, albedo.rgb, metallic, roughness);
    albedo *= lit.Diffuse;
    //ambient *= lit.Ambient;
    //metallic *= lit.Specular;
    //return float4(lit.Diffuse.rgb, 1);
//#else 
    //shadow = -N.z;
//#endif // ENABLE_LIGHTING
    
    float3 ambient = (float3) 0.03f * diffuse * ao;
    
    float3 color = ambient + albedo.xyz;
    
    //color = color / (color + (float3) (1.0));
    //color = pow(color, (float3) (1.0 / 2.2));

    return float4(color * shadow, alpha);
    //return float4(albedo.rgb, alpha);
}

