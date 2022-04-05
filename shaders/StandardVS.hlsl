#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"

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

    matrix mvp = mul(gViewProj, gWorld);
    VSOUT.Position = mul(mvp, float4(IN.Position, 1.0f));
    VSOUT.PositionWS = mul(gWorld, float4(IN.Position, 1.0f));
    VSOUT.NormalWS = mul((float3x3) gInverseTransposeWorld, IN.Normal);
    VSOUT.TexCoord = IN.TexCoord.xy;
    VSOUT.TangentWS = mul((float3x3) gInverseTransposeWorld, IN.Tangent);
    VSOUT.BitangentWS = mul((float3x3) gInverseTransposeWorld, IN.Bitangent);

    return VSOUT;
}