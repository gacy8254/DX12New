#include "MainPassCB.hlsli"

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float4 PositionWS : POSITION; //����ռ�����
};

float main(VertexOutput input) : SV_Target
{

//	clip(diffuseAlbedo.a - 0.1f);

    float lightDistance = length(input.PositionWS.xyz - gCameraPos.xyz);
	
    return lightDistance;
}
