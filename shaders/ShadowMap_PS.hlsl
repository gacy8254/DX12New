#include "MainPassCB.hlsli"

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float4 PositionWS : POSITION; //世界空间坐标
};

float main(VertexOutput input) : SV_Target
{
    float lightDistance = length(input.PositionWS.xyz - gCameraPos.xyz);

    return lightDistance;
}
