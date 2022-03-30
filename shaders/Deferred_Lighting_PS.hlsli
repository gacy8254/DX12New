// clang-format off
struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

#if ENABLE_LIGHTING
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

ConstantBuffer<LightProperties> LightPropertiesCB : register( b0 );

StructuredBuffer<PointLight> PointLights : register( t0 );
StructuredBuffer<SpotLight> SpotLights : register( t1 );
StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 );
#endif // ENABLE_LIGHTING

// Textures
Texture2D AlbedoText : register(t3);
Texture2D NormalText : register(t4);
Texture2D ORMText : register(t5);
Texture2D EmissiveText : register(t6);
Texture2D WorldPosText : register(t7);

SamplerState TextureSampler : register(s0);

#if ENABLE_LIGHTING
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

float DoAttenuation( float c, float l, float q, float d )
{
    return 1.0f / ( c + l * d + q * d * d );
}

float DoSpotCone( float3 spotDir, float3 L, float spotAngle )
{
    float minCos = cos( spotAngle );
    float maxCos = ( minCos + 1.0f ) / 2.0f;
    float cosAngle = dot( spotDir, -L );
    return smoothstep( minCos, maxCos, cosAngle );
}

LightResult DoPointLight( PointLight light, float3 V, float3 P, float3 N, float specularPower )
{
    LightResult result;
    float3 L = ( light.PositionVS.xyz - P );
    float d = length( L );
    L = L / d;

    float attenuation = DoAttenuation( light.ConstantAttenuation,
                                       light.LinearAttenuation,
                                       light.QuadraticAttenuation,
                                       d );

    result.Diffuse = DoDiffuse( N, L ) * attenuation * light.Color;
    result.Specular = DoSpecular( V, N, L, specularPower ) * attenuation * light.Color;
    result.Ambient = light.Color * light.Ambient;

    return result;
}

LightResult DoSpotLight( SpotLight light, float3 V, float3 P, float3 N, float specularPower )
{
    LightResult result;
    float3 L = ( light.PositionVS.xyz - P );
    float d = length( L );
    L = L / d;

    float attenuation = DoAttenuation( light.ConstantAttenuation,
                                       light.LinearAttenuation,
                                       light.QuadraticAttenuation,
                                       d );

    float spotIntensity = DoSpotCone( light.DirectionVS.xyz, L, light.SpotAngle );

    result.Diffuse = DoDiffuse( N, L ) * attenuation * spotIntensity * light.Color;
    result.Specular = DoSpecular( V, N, L, specularPower ) * attenuation * spotIntensity * light.Color;
    result.Ambient = light.Color * light.Ambient;

    return result;
}

LightResult DoDirectionalLight( DirectionalLight light, float3 V, float3 P, float3 N, float specularPower )
{
    LightResult result = (LightResult)0;

    float3 L = normalize( -light.DirectionVS.xyz );

    result.Diffuse = light.Color * DoDiffuse( N, L );
    result.Specular = light.Color * DoSpecular( V, N, L, specularPower );
    result.Ambient = light.Color * light.Ambient;

    return result;
}

LightResult DoLighting( float3 P, float3 N, float specularPower )
{
    uint i;

    // Lighting is performed in view space.
    float3 V = normalize( -P );

    LightResult totalResult = (LightResult)0;

    // Iterate point lights.
    for ( i = 0; i < LightPropertiesCB.NumPointLights; ++i )
    {
        LightResult result = DoPointLight( PointLights[i], V, P, N, specularPower );

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    // Iterate spot lights.
    for ( i = 0; i < LightPropertiesCB.NumSpotLights; ++i )
    {
        LightResult result = DoSpotLight( SpotLights[i], V, P, N, specularPower );

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    // Iterate directinal lights
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        LightResult result = DoDirectionalLight( DirectionalLights[i], V, P, N, specularPower );

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
        totalResult.Ambient += result.Ambient;
    }

    totalResult.Diffuse = saturate( totalResult.Diffuse );
    totalResult.Specular = saturate( totalResult.Specular );
    totalResult.Ambient = saturate( totalResult.Ambient );

    return totalResult;
}
#endif // ENABLE_LIGHTING

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
    
    float shadow = 1;
    float4 specular = 0;
#if ENABLE_LIGHTING
    LightResult lit = DoLighting( WorldPos, normal, roughness );
    albedo *= lit.Diffuse;
    //ambient *= lit.Ambient;
    metallic *= lit.Specular;
    //return float4(lit.Diffuse.rgb, 1);
#else 
    //shadow = -N.z;
#endif // ENABLE_LIGHTING

    return float4((emissive + albedo + specular).rgb * shadow, alpha);
    //return float4(roughness, roughness,roughness, 1);
}

