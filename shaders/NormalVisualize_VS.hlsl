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
    float4 PositionVS : POSITION; //�۲�ռ�����
    float3 NormalVS : NORMAL; //�۲�ռ䷢��
    matrix ModelMat : MODEL;
    matrix ModelViewPorjMat : VIEWPROJ;
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;

    VSOUT.PositionVS = float4(IN.Position, 1.0f);
    VSOUT.NormalVS = IN.Normal;
    VSOUT.ModelMat = gWorld;
    float4x4 mvp = mul(gViewProj, gWorld);
    VSOUT.ModelViewPorjMat = mvp;

    return VSOUT;
}