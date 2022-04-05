#include "ShaderDefinition.h"

//struct MainPass
//{
//    matrix gView;
//    matrix gInverseView;
//    matrix gProj;
//    matrix gInverseProj;
//    matrix gUnjitteredProj;
//    matrix gUnjitteredInverseProj;
//    matrix gInverseViewProj;
//    matrix gViewProj;
//    float4 gCameraPos;
//    float gJitterX;
//    float gJitterY;
//    float gTotalTime;
//    float gDeltaTime;
//    float gNearZ;
//    float gFarZ;
//    uint gFrameCount;
//    float gPad;
//};
//ConstantBuffer<MainPass> MainPassCB : register(b1);

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInverseView;
    float4x4 gProj;
    float4x4 gInverseProj;
    float4x4 gUnjitteredProj;
    float4x4 gUnjitteredInverseProj;
    float4x4 gInverseViewProj;
    float4x4 gViewProj;
    float4x4 UnjitteredViewProj;
    float4x4 gViewProjTex;
    float4 gCameraPos;
    float2 gJitter;
    float gTotalTime;
    float gDeltaTime;
    float gNearZ;
    float gFarZ;
    uint gFrameCount;
    float gPad;
   
    
}