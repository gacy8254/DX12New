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
    float4 Position             : SV_Position;  //剪裁空间坐标
    float4 PositionWS           : POSITION;     //世界空间坐标
    float4 CurrentPosition      : POSITION1;    //当前的剪裁空间坐标,使用未抖动的矩阵
    float4 PreviousPosition     : POSITION2;    //上一帧的剪裁空间坐标
    float3 NormalWS             : NORMAL;       //世界空间法线
    float3 TangentWS            : TANGENT;      //切线
    float3 BitangentWS          : BITANGENT;    //副切线
    float2 TexCoord             : TEXCOORD;     //UV
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;

    //计算剪裁空间坐标
    matrix mvp = mul(gViewProj, gWorld);
    VSOUT.Position = mul(mvp, float4(IN.Position, 1.0f));
    
    //当前的世界坐标
    float4 worldPos = mul(gWorld, float4(IN.Position, 1.0f));
    //上一帧的世界坐标
    float4 previousWorldPos = mul(gPreviousWorld, float4(IN.Position, 1.0f));
    
    //当前的剪裁空间坐标,使用未抖动的矩阵
    VSOUT.CurrentPosition = mul(gUnjitteredViewProj, worldPos);
    //上一帧的剪裁空间坐标
    VSOUT.PreviousPosition = mul(gPreviousViewProj, previousWorldPos);
    
    VSOUT.PositionWS = worldPos;
    
    //法线和UV
    VSOUT.NormalWS = mul((float3x3) gInverseTransposeWorld, IN.Normal);
    VSOUT.TangentWS = mul((float3x3) gInverseTransposeWorld, IN.Tangent);
    VSOUT.BitangentWS = mul((float3x3) gInverseTransposeWorld, IN.Bitangent);
    VSOUT.TexCoord = IN.TexCoord.xy;

    return VSOUT;
}