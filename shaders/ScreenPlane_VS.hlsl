struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float2 TexC : TEXCOORD;
    float4 PosH : SV_POSITION;
};

VertexOut main(VertexInput input)
{
    VertexOut output;

    output.TexC = input.TexCoord;

    output.PosH = float4(input.Position, 1.0f);

	// transform to homogeneous clip space.
    output.PosH.x = 2 * output.PosH.x - 1;
    output.PosH.y = 2 * output.PosH.y - 1;

    return output;
}