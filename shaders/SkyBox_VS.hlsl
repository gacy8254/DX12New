struct Mat
{
    matrix ModelMat;
    matrix ModelViewMat;
    matrix InverseTransposeModelViewMat;
    matrix ModelViewPorjMat;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float3 TexCoord : TEXCOORD;       //UV
    float4 Position : SV_Position;    //���ÿռ�����
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;
    
    VSOUT.Position = mul(MatCB.ModelViewPorjMat, float4(IN.Position, 1.0f));
    VSOUT.TexCoord = IN.Position;

    return VSOUT;
}