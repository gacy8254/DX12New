struct Mat
{
    matrix viewProjMat;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexInput
{
    float3 Position : POSITION;
};

struct VertexOutput
{
    float3 TexCoord : TEXCOORD;       //UV
    float4 Position : SV_Position;    //¼ô²Ã¿Õ¼ä×ø±ê
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;
    
    VSOUT.Position = mul(MatCB.viewProjMat, float4(IN.Position, 1.0f));
    VSOUT.TexCoord = IN.Position;

    return VSOUT;
}