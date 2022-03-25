struct VSOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

VSOutput main(uint VertexID : SV_VertexID)
{
    VSOutput OUT;
    
    OUT.TexCoord = float2(uint2(VertexID, VertexID << 1) & 2);
    OUT.Position = float4(lerp(float2(-1, 1), float2(1, -1), OUT.TexCoord), 0, 1);
    
    return OUT;
}