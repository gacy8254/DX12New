
struct Mat
{
    matrix ModelMat;
    matrix ModelViewMat;
    matrix InverseTransposeModelMat;
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
    float4 PositionWS : POSITION;   //世界空间坐标
    float3 NormalWS : NORMAL;       //世界空间发现
    float3 TangentWS : TANGENT;     //切线
    float3 BitangentWS : BITANGENT; //副切线
    float2 TexCoord : TEXCOORD;     //UV
    float4 Position : SV_Position;  //剪裁空间坐标
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;

    VSOUT.Position = mul(MatCB.ModelViewPorjMat, float4(IN.Position, 1.0f));
    VSOUT.PositionWS = mul(MatCB.ModelMat, float4(IN.Position, 1.0f));
    VSOUT.NormalWS = mul((float3x3) MatCB.InverseTransposeModelMat, IN.Normal);
    VSOUT.TexCoord = IN.TexCoord.xy;
    VSOUT.TangentWS = mul((float3x3) MatCB.InverseTransposeModelMat, IN.Tangent);
    VSOUT.BitangentWS = mul((float3x3) MatCB.InverseTransposeModelMat, IN.Bitangent);

    return VSOUT;
}