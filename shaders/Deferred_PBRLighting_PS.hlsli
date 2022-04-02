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
TextureCube<float4> IrradianceText : register(t8);
TextureCube<float4> PrefilterText : register(t9);
Texture2D IntegrateBRDFText : register(t10);

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWarp : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWarp : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

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

float3 DoPointLight(PointLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness)
{
    float3 result;
    float3 toCamera = normalize(CameraProperticesCB.CameraPos.xyz - P);
    
    float3 toLight = normalize(light.PositionWS.xyz - P);
    float3 halfVec = normalize(toCamera + toLight);
    
    float dis = length(light.PositionWS.xyz - P);
    float attenuation = DoAttenuation(dis);
    
    float3 radiance = light.Color * attenuation;
    
    float NDF = DistributionGGX(N.xyz, halfVec, _roughness);
    float G = GeometrySmith(N.xyz, toCamera, toLight, _roughness);
    
    float3 F = CalFresnel(halfVec, toCamera, _albedo.rgb, _metallic);
    
    float3 numerator = NDF * G * F;
    float demoninator = 4.0 * max(dot(N.xyz, toCamera), 0.0f) * max(dot(N.xyz, toLight), 0.0) + 0.0001;
    
    float3 specular = numerator / demoninator;
    
    float3 KS = F;
    
    float3 kd = (float3) 1.0f - KS;
    
    kd *= 1.0f - _metallic;
    
    float NdotL = max(dot(N.xyz, toLight), 0.0);
    
    float3 lo = (kd * _albedo.rgb / PI + specular) * radiance * NdotL;
    
    result = lo;

    return result;
}

float3 DoSpotLight(SpotLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness)
{
    float3 result;
    
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
    
    result = Lo;

    return result;
}

float3 DoDirectionalLight(DirectionalLight light, float3 V, float3 P, float3 N, float3 _albedo, float _metallic, float _roughness)
{
    float3 result = (float3)0;

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
    result = Lo;


    return result;
}

float3 DoLighting(float3 P, float3 N,  float3 _camPos, float3 _albedo, float _metallic, float _roughness)
{
    uint i;

    // Lighting is performed in view space.
    float3 V = normalize(_camPos - P);

    float3 totalResult = (float3)0;

    // Iterate point lights.
    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        float3 result = DoPointLight(PointLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult += result;
    }

    // Iterate spot lights.
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        float3 result = DoSpotLight(SpotLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult += result;
    }

    // Iterate directinal lights
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        float3 result = DoDirectionalLight(DirectionalLights[i], V, P, N, _albedo, _metallic, _roughness);

        totalResult += result;
    }

    //totalResult = saturate( totalResult );

    return totalResult;
}
//#endif // ENABLE_LIGHTING

float4 main(PixelShaderInput IN) : SV_Target
{
    float3 WorldPos = WorldPosText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float4 albedo = AlbedoText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float ao = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).r;
    float roughness = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).g;
    float metallic = ORMText.Sample(SamAnisotropicWarp, IN.TexCoord.xy).b;
    float4 normal = NormalText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float4 emissive = EmissiveText.Sample(SamAnisotropicWarp, IN.TexCoord.xy);
    float alpha = albedo.a;
    
    float3 toCamera = normalize(CameraProperticesCB.CameraPos.xyz - WorldPos);
    
    float3 diffuse;
    float shadow = 1;
//#if ENABLE_LIGHTING
    float3 lit = DoLighting(WorldPos, normal.rgb, CameraProperticesCB.CameraPos.xyz, albedo.rgb, metallic, roughness);
    
    float3 reflectVec = reflect(-toCamera, normal.rgb);
    
    float3 F = CalFresnelRoughness(normal.rgb, toCamera, albedo.rgb, metallic, roughness);
    
    float3 KS = F;
    float3 KD = (float3) (1.0f) - KS;
    KD *= 1.0 - metallic;
    float3 irradiance = IrradianceText.Sample(SamAnisotropicWarp, normal.rgb).rgb;
    diffuse = irradiance * albedo.rgb;
    
    const float MAX_REFLECTION_LOD = 4.0F;
    float3 prefilterColor = PrefilterText.SampleLevel(SamAnisotropicClamp, reflectVec, roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = IntegrateBRDFText.SampleLevel(SamLinearClamp, float2(max(dot(normal.rgb, toCamera), 0.0f), roughness), 0.0f).rg;
    float3 specular = prefilterColor * (F * brdf.x + brdf.y);
    
    float3 ambient = (KD * diffuse + specular) * ao;
    float3 color = ambient + lit.rgb;
//#else 
    //shadow = -N.z;
//#endif // ENABLE_LIGHTING

    return float4(color * shadow, alpha);
    float3 ccc = (float3) (specular);
    //return float4(ccc, alpha);
}

