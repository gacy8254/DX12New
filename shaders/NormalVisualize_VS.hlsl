
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
    float4 PositionVS : POSITION; //观察空间坐标
    float3 NormalVS : NORMAL; //观察空间发现
    matrix ModelMat : MODEL;
    matrix ModelViewPorjMat : VIEWPROJ;
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;

    VSOUT.PositionVS = float4(IN.Position, 1.0f);
    VSOUT.NormalVS = IN.Normal;
    VSOUT.ModelMat = MatCB.ModelMat;
    VSOUT.ModelViewPorjMat = MatCB.ModelViewPorjMat;

    return VSOUT;
}