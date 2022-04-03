
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
    float4 PositionWS : POSITION;   //����ռ�����
    float3 NormalWS : NORMAL;       //����ռ䷢��
    float3 TangentWS : TANGENT;     //����
    float3 BitangentWS : BITANGENT; //������
    float2 TexCoord : TEXCOORD;     //UV
    float4 Position : SV_Position;  //���ÿռ�����
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