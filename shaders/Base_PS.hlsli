// clang-format off
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

ConstantBuffer<Material> MaterialCB : register(b0, space1);

float4 main(PixelShaderInput IN) : SV_Target
{
    Material material = MaterialCB;

    return float4(material.Diffuse.rgb, 1.0f);
}

