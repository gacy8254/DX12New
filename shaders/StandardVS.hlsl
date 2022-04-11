#include "ObjectCB.hlsli"
#include "MainPassCB.hlsli"

struct VertexInput
{
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float3 Tangent      : TANGENT;
    float3 Bitangent    : BITANGENT;
    float3 TexCoord     : TEXCOORD;
};

struct VertexOutput
{
    float4 Position             : SV_Position;  //���ÿռ�����
    float4 PositionWS           : POSITION;     //����ռ�����
    float4 CurrentPosition      : POSITION1;    //��ǰ�ļ��ÿռ�����,ʹ��δ�����ľ���
    float4 PreviousPosition     : POSITION2;    //��һ֡�ļ��ÿռ�����
    float3 NormalWS             : NORMAL;       //����ռ䷨��
    float3 TangentWS            : TANGENT;      //����
    float3 BitangentWS          : BITANGENT;    //������
    float2 TexCoord             : TEXCOORD;     //UV
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;

    //������ÿռ�����
    matrix mvp = mul(gViewProj, gWorld);
    VSOUT.Position = mul(mvp, float4(IN.Position, 1.0f));
    
    //��ǰ����������
    float4 worldPos = mul(gWorld, float4(IN.Position, 1.0f));
    //��һ֡����������
    float4 previousWorldPos = mul(gPreviousWorld, float4(IN.Position, 1.0f));
    
    //��ǰ�ļ��ÿռ�����,ʹ��δ�����ľ���
    VSOUT.CurrentPosition = mul(gUnjitteredViewProj, worldPos);
    //��һ֡�ļ��ÿռ�����
    VSOUT.PreviousPosition = mul(gPreviousViewProj, previousWorldPos);
    
    VSOUT.PositionWS = worldPos;
    
    //���ߺ�UV
    VSOUT.NormalWS = mul((float3x3) gInverseTransposeWorld, IN.Normal);
    VSOUT.TangentWS = mul((float3x3) gInverseTransposeWorld, IN.Tangent);
    VSOUT.BitangentWS = mul((float3x3) gInverseTransposeWorld, IN.Bitangent);
    VSOUT.TexCoord = IN.TexCoord.xy;

    return VSOUT;
}