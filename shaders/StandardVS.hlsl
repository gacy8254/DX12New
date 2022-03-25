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
    float2 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float4 PositionVS : POSITION;    //观察空间坐标
    float3 NormalVS : NORMAL;         //观察空间发现
    float2 TexCoord : TEXCOORD;       //UV
    float4 Position : SV_Position;    //剪裁空间坐标
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;
    
    VSOUT.Position = mul(MatCB.ModelViewPorjMat, float4(IN.Position, 1.0f));
    VSOUT.PositionVS = mul(MatCB.ModelViewMat, float4(IN.Position, 1.0f));
    VSOUT.NormalVS = mul((float3x3) MatCB.InverseTransposeModelViewMat, IN.Normal);
    VSOUT.TexCoord = IN.TexCoord;

    return VSOUT;
}