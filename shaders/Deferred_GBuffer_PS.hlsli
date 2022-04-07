struct PixelShaderInput
{
    float4 Position             : SV_Position; //剪裁空间坐标
    float4 PositionWS           : POSITION; //世界空间坐标
    float4 CurrentPosition      : POSITION1; //当前的剪裁空间坐标,使用未抖动的矩阵
    float4 PreviousPosition     : POSITION2; //上一帧的剪裁空间坐标
    float3 NormalWS             : NORMAL; //世界空间法线
    float3 TangentWS            : TANGENT; //切线
    float3 BitangentWS          : BITANGENT; //副切线
    float2 TexCoord             : TEXCOORD; //UV
    
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
    bool HasAOTexture;
    bool HasEmissiveTexture;
    bool HasDiffuseTexture;
    bool HasMetalticTexture;
    //------------------------------------ ( 16 bytes )
    bool HasRoughnessTexture;
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
    float2 Velocity      : SV_TARGET5;
    //float4 Albedo : SV_TARGET0;
};

ConstantBuffer<Material> MaterialCB : register(b0, space1);

// Textures
Texture2D ORMTexture : register(t0);
Texture2D EmissiveTexture : register(t1);
Texture2D DiffuseTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D BumpTexture : register(t4);
Texture2D OpacityTexture : register(t5);

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
    
    float gamma = 2.2;

    if (material.HasAOTexture)
    {
        AO = ORMTexture.Sample(TextureSampler, uv).r;
        roughness = ORMTexture.Sample(TextureSampler, uv).g;
        metaltic = ORMTexture.Sample(TextureSampler, uv).b;
    }
    else
    {
        AO = material.Specular.r;
        roughness = material.Specular.g;
        metaltic = material.Specular.b;
    }
    
    if (material.HasEmissiveTexture)
    {
        emissive = EmissiveTexture.Sample(TextureSampler, uv);
        emissive.rgb = pow(emissive.rgb, gamma);
    }
    else
    {
        emissive = material.Emissive;
    }
    
    if (material.HasDiffuseTexture)
    {
        diffuse = DiffuseTexture.Sample(TextureSampler, uv);
        diffuse.rgb = pow(diffuse.rgb, gamma);
    }
    else
    {
        diffuse = material.Diffuse;
    }
    
    //if (material.HasRoughnessTexture)
    //{
        
    //    roughness = ORMTexture.Sample(TextureSampler, uv).g;
    //}
    //else
    //{
    //    roughness = material.Specular.g;
    //}
    //if(material.HasMetalticTexture)
    //{
        
    //    metaltic = ORMTexture.Sample(TextureSampler, uv).b;
    //}
    //else
    //{
       
    //   metaltic = material.Specular.b;
    //}
    
    float3 N;
    // Normal mapping
    if (material.HasNormalTexture)
    {
        float3 tangent = normalize(IN.TangentWS);
        float3 bitangent = normalize(IN.BitangentWS);
        float3 normal = normalize(IN.NormalWS);

        float3x3 TBN = float3x3(tangent,
                                 bitangent,
                                 normal);

        N = DoNormalMapping(TBN, NormalTexture, uv);
    }
    else if (material.HasBumpTexture)
    {
        float3 tangent = normalize(IN.TangentWS);
        float3 bitangent = normalize(IN.BitangentWS);
        float3 normal = normalize(IN.NormalWS);

        float3x3 TBN = float3x3(tangent,
                                 bitangent,
                                 normal);

        N = DoNormalMapping(TBN, NormalTexture, uv);
    }
    else
    {
        N = normalize(IN.NormalWS);
    }
    
    
    //速度
    float4 previousPos = IN.PreviousPosition;
    previousPos = previousPos / previousPos.w;
    previousPos.xy = previousPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);// 翻转Y轴,因为世界坐标和贴图坐标有不同的Y轴
    
    float4 currentPos = IN.CurrentPosition;
    currentPos = currentPos / currentPos.w;
    currentPos.xy = currentPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);
    
    Out.Velocity = float2(currentPos.x - previousPos.x, currentPos.y - previousPos.y);
    Out.Albedo = diffuse;
    Out.Albedo.a = alpha;
    Out.Normal.rgb = N;
    Out.ORM.r = AO;
    Out.ORM.g = roughness;
    Out.ORM.b = metaltic;
    Out.Emissive = emissive;
    Out.WorldPos.rgb = IN.PositionWS;
    
    return Out;
}

