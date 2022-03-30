struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS : NORMAL;
    float3 TangentVS : TANGENT;
    float3 BitangentVS : BITANGENT;
    float2 TexCoord : TEXCOORD;
};

struct Material
{
    float4 Diffuse;
    //------------------------------------ ( 16 bytes )
    float4 Specular;
    //------------------------------------ ( 16 bytes )
    float4 Emissive;
    //------------------------------------ ( 16 bytes )
    float4 Ambient;
    //------------------------------------ ( 16 bytes )
    float4 Reflectance;
    //------------------------------------ ( 16 bytes )
    float Opacity; // If Opacity < 1, then the material is transparent.
    float SpecularPower;
    float IndexOfRefraction; // For transparent materials, IOR > 0.
    float BumpIntensity; // When using bump textures (height maps) we need
                              // to scale the height values so the normals are visible.
    //------------------------------------ ( 16 bytes )
    bool HasAmbientTexture;
    bool HasEmissiveTexture;
    bool HasDiffuseTexture;
    bool HasSpecularTexture;
    //------------------------------------ ( 16 bytes )
    bool HasSpecularPowerTexture;
    bool HasNormalTexture;
    bool HasBumpTexture;
    bool HasOpacityTexture;
    //------------------------------------ ( 16 bytes )
    // Total:                              ( 16 * 8 = 128 bytes )
};

struct PixelOut
{
    float4 Albedo        : SV_TARGET0;
    float4 Normal        : SV_TARGET1;
    float4 ORM           : SV_TARGET2;
    float4 Emissive      : SV_TARGET3;
    float4 WorldPos      : SV_TARGET4;
    //float4 Albedo : SV_TARGET0;
};

ConstantBuffer<Material> MaterialCB : register(b0, space1);

// Textures
Texture2D AOTexture : register(t0);
Texture2D EmissiveTexture : register(t1);
Texture2D DiffuseTexture : register(t2);
Texture2D MetalticTexture : register(t3);
Texture2D RoughnessTexture : register(t4);
Texture2D NormalTexture : register(t5);
Texture2D BumpTexture : register(t6);
Texture2D OpacityTexture : register(t7);

SamplerState TextureSampler : register(s0);

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
    //return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

    // This is cheaper but nearly equivalent
    return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

float3 ExpandNormal(float3 n)
{
    return n * 2.0f - 1.0f;
}

float3 DoNormalMapping(float3x3 TBN, Texture2D tex, float2 uv)
{
    float3 N = tex.Sample(TextureSampler, uv).xyz;
    N = ExpandNormal(N);

    // Transform normal from tangent space to view space.
    N = mul(N, TBN);
    return normalize(N);
}

float3 DoBumpMapping(float3x3 TBN, Texture2D tex, float2 uv, float bumpScale)
{
    // Sample the heightmap at the current texture coordinate.
    float height_00 = tex.Sample(TextureSampler, uv).r * bumpScale;
    // Sample the heightmap in the U texture coordinate direction.
    float height_10 = tex.Sample(TextureSampler, uv, int2(1, 0)).r * bumpScale;
    // Sample the heightmap in the V texture coordinate direction.
    float height_01 = tex.Sample(TextureSampler, uv, int2(0, 1)).r * bumpScale;

    float3 p_00 = { 0, 0, height_00 };
    float3 p_10 = { 1, 0, height_10 };
    float3 p_01 = { 0, 1, height_01 };

    // normal = tangent x bitangent
    float3 tangent = normalize(p_10 - p_00);
    float3 bitangent = normalize(p_01 - p_00);

    float3 normal = cross(tangent, bitangent);

    // Transform normal from tangent space to view space.
    normal = mul(normal, TBN);

    return normal;
}

// If c is not black, then blend the color with the texture
// otherwise, replace the color with the texture.
float4 SampleTexture(Texture2D t, float2 uv, float4 c)
{
    if (any(c.rgb))
    {
        c *= t.Sample(TextureSampler, uv);
    }
    else
    {
        c = t.Sample(TextureSampler, uv);
    }

    return c;
}

PixelOut main(PixelShaderInput IN)
{
    PixelOut Out;
    
    Material material = MaterialCB;

    // By default, use the alpha component of the diffuse color.
    float alpha = material.Diffuse.a;
    if (material.HasOpacityTexture)
    {
        alpha = OpacityTexture.Sample(TextureSampler, IN.TexCoord.xy).r;
    }

#if ENABLE_DECAL
    if ( alpha < 0.1f )
    {
        discard; // Discard the pixel if it is below a certain threshold.
    }
#endif // ENABLE_DECAL

    float AO = 1.0f;
    float4 emissive = (float4) 0;
    float4 diffuse = (float4)1;
    float metaltic = 0;
    float roughness = 1.0f;
    float2 uv = IN.TexCoord.xy;

    if (material.HasAmbientTexture)
    {
        AO = AOTexture.Sample(TextureSampler, uv).r;
    }
    if (material.HasEmissiveTexture)
    {
        emissive = EmissiveTexture.Sample(TextureSampler, uv);
    }
    if (material.HasDiffuseTexture)
    {
        diffuse = DiffuseTexture.Sample(TextureSampler, uv);
    }
    if (material.HasSpecularPowerTexture)
    {
        roughness = RoughnessTexture.Sample(TextureSampler, uv).r;
    }
    if(material.HasSpecularTexture)
    {
        metaltic = MetalticTexture.Sample(TextureSampler, uv).r;

    }
    float3 N;
    // Normal mapping
    if (material.HasNormalTexture)
    {
        float3 tangent = normalize(IN.TangentVS);
        float3 bitangent = normalize(IN.BitangentVS);
        float3 normal = normalize(IN.NormalVS);

        float3x3 TBN = float3x3(tangent,
                                 bitangent,
                                 normal);

        N = DoNormalMapping(TBN, NormalTexture, uv);
    }
    else if (material.HasBumpTexture)
    {
        float3 tangent = normalize(IN.TangentVS);
        float3 bitangent = normalize(IN.BitangentVS);
        float3 normal = normalize(IN.NormalVS);

        float3x3 TBN = float3x3(tangent,
                                 -bitangent,
                                 normal);

        N = DoBumpMapping(TBN, BumpTexture, uv, material.BumpIntensity);
    }
    else
    {
        N = normalize(IN.NormalVS);
    }
    
    Out.Albedo = diffuse;
    Out.Albedo.a = alpha;
    Out.Normal.rgb = N;
    Out.ORM.r = AO;
    Out.ORM.g = roughness;
    Out.ORM.b = metaltic;
    Out.Emissive = emissive;
    Out.WorldPos.rgb = IN.PositionVS;
    
    return Out;
}

