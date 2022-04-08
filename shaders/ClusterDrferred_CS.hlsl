
#ifndef CLUSTERED_DEFERRED_CS_HLSL
#define CLUSTERED_DEFERRED_CS_HLSL
//深度分片划分
static const float DepthSlicing_16[17] =
{
    1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};

#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

RWStructuredBuffer<LightList> gLightList : register(u0);

//Texture2D gDepthBuffer				: register(t0);

[numthreads(CLUSTER_THREAD_NUM_X, CLUSTER_THREAD_NUM_Y, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID
	)
{
    uint groupIndex = groupThreadId.y * CLUSTER_THREAD_NUM_X + groupThreadId.x;

    uint2 globalCoords = dispatchThreadId.xy;

    uint clusterIdStart = (groupId.y * ceil(gRTSize.x / CLUSTER_SIZE_X) + groupId.x) * CLUSTER_NUM_Z;

    // Initialize light list.
    if (groupIndex < CLUSTER_NUM_Z)
    {
        gLightList[clusterIdStart + groupIndex].NumPointLights = 0;
        gLightList[clusterIdStart + groupIndex].NumSpotLights = 0;
    }

    GroupMemoryBarrierWithGroupSync();

	// Work out scale/bias from [0, 1]
    float2 tileNum = float2(gRTSize.xy) * rcp(float2(CLUSTER_SIZE_X, CLUSTER_SIZE_Y));
    float2 tileCenterOffset = float2(groupId.xy) * 2 + float2(1.0f, 1.0f) - tileNum;

	// Now work out composite projection matrix
	// Relevant matrix columns for this tile frusta
    float4 c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
    float4 c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Derive frustum planes
    float4 frustumPlanes[6];
    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Normalize frustum planes (near/far already normalized)
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }

    // Cull lights for this cluster
    for (uint lightIndex = groupIndex; lightIndex < (uint) LightPropertiesCB.NumPointLights; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
    {
        bool inFrustum = true;

        float4 lightPositionView = mul(gView, float4(PointLights[lightIndex].PositionWS.xyz, 1.0f));
       // float4 lightPositionView = mul(float4(PointLights[lightIndex].PositionWS.xyz, 1.0f), gView);

		[unroll]
        for (uint i = 0; i < 4; ++i)
        {
            float d = dot(frustumPlanes[i], lightPositionView);
            inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
        }

        if (inFrustum)
        {
			[unroll]
            for (uint clusterZ = 0; clusterZ < CLUSTER_NUM_Z; clusterZ++)
            {
				// Cull: point light sphere vs cluster frustum
                inFrustum = true;

				// Near/far
                frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_16[clusterZ]);
                frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_16[clusterZ + 1]);

				[unroll]
                for (uint i = 4; i < 6; ++i)
                {
                    float d = dot(frustumPlanes[i], lightPositionView);
                    inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
                }

				[branch]
                if (inFrustum)
                {
					// Append light to list
					// Compaction might be better if we expect a lot of lights
                    uint listIndex = 0;
                    InterlockedAdd(gLightList[clusterIdStart + clusterZ].NumPointLights, 1, listIndex);
                    if (listIndex < MAX_GRID_POINT_LIGHT_NUM)
                    {
                        gLightList[clusterIdStart + clusterZ].PointlightIndices[listIndex] = lightIndex;
                    }
                }
            }
        }

    }
}


#endif




//#include "Lighting.hlsli"
//#include "MainPassCB.hlsli"

//RWStructuredBuffer<LightList> gLightList : register(u0);

////深度分片划分
//static const float DepthSlicing_16[17] =
//{
//    1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
//	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
//	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
//	5000.0f, 50000.0f
//};


//[numthreads(CLUSTER_THREAD_NUM_X, CLUSTER_THREAD_NUM_Y, 1)]
//void main( 
//uint3 groupID           : SV_DispatchThreadID,
//uint3 dispatchThreadID  :SV_DispatchThreadID,
//uint3 groupThreadID     :SV_GroupThreadID
//)
//{
//    //计算线程组内线程的序号,这是一个一维数值
//    uint groupIndex = groupThreadID.y * CLUSTER_THREAD_NUM_X + groupThreadID.x;
    
//    //计算线程在全局范围内的二维索引
//    uint2 globalCoords = dispatchThreadID.xy;
    
//    //计算分簇的起始位置
//    uint clusterStart = (groupID.y * ceil(gRTSize.x / CLUSTER_SIZE_X) + groupID.x) * CLUSTER_NUM_Z;
    
//    //对灯光列表初始化
//    if (groupIndex < CLUSTER_NUM_Z)
//    {
//        gLightList[clusterStart + groupIndex].NumPointLights = 0;
//        gLightList[clusterStart + groupIndex].NumSpotLights = 0;
//    }
    
//    //等待所有线程完成计算
//    GroupMemoryBarrierWithGroupSync();
    
//    //计算出 比例/偏差 从[0, 1]
//    //计算屏幕分片的数量使用RT的大小除以分簇的数量
//    float2 tileNum = float2(gRTSize.xy) * rcp(float2(CLUSTER_SIZE_X, CLUSTER_SIZE_Y));
//    //计算当前分片到中心的偏移量
//    float2 tileCenterOffset = float2(groupID.xy) * 2 + float2(1.0f, 1.0f) - tileNum;
    
//    //计算投影矩阵
//    float c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
//    float c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
//    float c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    
//    //计算视椎体的六个面
//    float4 frustumPlanes[6];

//    //侧面
//    frustumPlanes[0] = c4 - c1;
//    frustumPlanes[1] = c4 + c1;
//    frustumPlanes[2] = c4 - c2;
//    frustumPlanes[3] = c4 + c1;
    
//    //归一化
//    [unroll]
//    for (uint i = 0; i < 4; ++i)
//    {
//        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
//    }

//    //从当前分簇中剔除灯光
//    for (uint lightIndex = groupIndex; lightIndex < (uint) LightPropertiesCB.NumPointLights; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
//    {
//        bool inFrustum = true;
        
//        //计算灯光在观察空间中的坐标
//        float4 lightPositionView = mul(gView, float4(PointLights[lightIndex].PositionWS.xyz, 1.0f));

//        [unroll]
//        for (uint i = 0; i < 4; i++)
//        {
//            //判断灯光是否能影响到当前分簇(上下左右)
//            float d = dot(frustumPlanes[i], lightPositionView);
//            inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
//        }
        
//        if (inFrustum)
//        {
//            [unroll]
//            for (uint clusterZ = 0; clusterZ < CLUSTER_NUM_Z; clusterZ++)
//            {
//                //视锥的前后面
//                inFrustum = true;
//                frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_16[clusterZ]);
//                frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_16[clusterZ + 1]);
                
//                //判断灯光是否能影响到当前分簇(前后)
//                [unroll]
//                for (uint i = 4; i < 6; ++i)
//                {
//                    float d = dot(frustumPlanes[i], lightPositionView);
//                    inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
//                }

//                [branch]
//                if(inFrustum)
//                {
//                    uint listIndex = 0;
                    
//                    InterlockedAdd(gLightList[clusterStart + clusterZ].NumPointLights, 1, listIndex);
//                    if(listIndex < MAX_GRID_POINT_LIGHT_NUM)
//                    {
//                        gLightList[clusterStart + clusterZ].PointlightIndices[listIndex] = lightIndex;
//                    }
//                }
                
//            }
//        }

//    }
//}