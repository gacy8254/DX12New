#include "ShaderDefinition.h"
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
    float3 TexCoord : TEXCOORD;       //UV
    float4 Position : SV_Position;    //¼ô²Ã¿Õ¼ä×ø±ê
};

VertexOutput main(VertexInput IN)
{
    VertexOutput VSOUT;
    float4x4 mvp = mul(ObjectCB.gWorld, MainPassCB.gViewProj);
    VSOUT.Position = mul(mvp, float4(IN.Position, 1.0f));
    VSOUT.Position.z = VSOUT.Position.w * FAR_Z_NORM;
    VSOUT.TexCoord = IN.Position;

    return VSOUT;
}