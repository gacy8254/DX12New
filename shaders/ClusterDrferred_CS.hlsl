#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

RWStructuredBuffer<LightList> gLightList : register(u0);

//深度分片划分
static const float DepthSlicing_16[17] =
{
    1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};

static const float DepthSlicing_64[65] =
{
    1, 2, 3, 5, 8, 12, 17, 23, 30, 38, 47, 57, 68, 70, 83, 97,
    117, 147, 187, 237, 297, 367, 447, 537, 637, 747, 867, 997, 1137, 1287, 1447, 1617,
    1778, 1965, 2152, 2367, 2604, 2864, 3151, 3466, 3812, 4194, 4613, 5074, 5582, 6140, 6754, 7430,
    8544, 9826, 11300, 12995, 14944, 17186, 19764, 22728, 26138, 30058, 34567, 39752, 45715, 46715, 47715, 48715,
    50000.0f
};

[numthreads(CLUSTER_THREAD_NUM_X, CLUSTER_THREAD_NUM_Y, 1)]
void main(
uint3 groupID : SV_GroupID,
uint3 dispatchThreadID : SV_DispatchThreadID,
uint3 groupThreadID : SV_GroupThreadID
)
{
    //计算线程组内线程的序号,这是一个一维数值
    uint groupIndex = groupThreadID.y * CLUSTER_THREAD_NUM_X + groupThreadID.x;
    
    //计算线程在全局范围内的二维索引
    uint2 globalCoords = dispatchThreadID.xy;
    
    //计算分簇的起始位置
    uint clusterStart = (groupID.y * ceil(gRTSize.x / CLUSTER_SIZE_X) + groupID.x) * CLUSTER_NUM_Z;
    
    //对灯光列表初始化
    if (groupIndex < CLUSTER_NUM_Z)
    {
        gLightList[clusterStart + groupIndex].NumPointLights = 0;
        gLightList[clusterStart + groupIndex].NumSpotLights = 0;
    }
    
    //等待所有线程完成计算
    GroupMemoryBarrierWithGroupSync();
    
    //计算出 比例/偏差 从[0, 1]
    //计算屏幕分片的数量使用RT的大小除以分簇的数量
    float2 tileNum = float2(gRTSize.xy) * rcp(float2(CLUSTER_SIZE_X, CLUSTER_SIZE_Y));
    //计算当前分片到中心的偏移量
    float2 tileCenterOffset = float2(groupID.xy) * 2 + float2(1.0f, 1.0f) - tileNum;
    
    //计算投影矩阵
    float4 c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
    float4 c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    
    //计算视椎体的六个面
    float4 frustumPlanes[6];

    //侧面
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;
    
    //归一化
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }

    //从当前分簇中剔除灯光
    for (uint lightIndex = groupIndex; lightIndex < (uint) LightPropertiesCB.NumPointLights; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
    {
        bool inFrustum = true;
        
        //计算灯光在观察空间中的坐标
        float4 lightPositionView = mul(gView, float4(PointLights[lightIndex].PositionWS.xyz, 1.0f));

        [unroll]
        for (uint i = 0; i < 4; i++)
        {
            //判断灯光是否能影响到当前分簇(上下左右)
            float d = dot(frustumPlanes[i], lightPositionView);
            inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
        }
        
        if (inFrustum)
        {
            [unroll]
            for (uint clusterZ = 0; clusterZ < CLUSTER_NUM_Z; clusterZ++)
            {
                //视锥的前后面
                inFrustum = true;
                frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_64[clusterZ]);
                frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_64[clusterZ + 1]);
                
                //判断灯光是否能影响到当前分簇(前后)
                [unroll]
                for (uint i = 4; i < 6; ++i)
                {
                    float d = dot(frustumPlanes[i], lightPositionView);
                    inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
                }

                [branch]
                if (inFrustum)
                {
                    uint listIndex = 0;
                    
                    InterlockedAdd(gLightList[clusterStart + clusterZ].NumPointLights, 1, listIndex);
                    if (listIndex < MAX_GRID_POINT_LIGHT_NUM)
                    {
                        gLightList[clusterStart + clusterZ].PointlightIndices[listIndex] = lightIndex;
                    }
                }
                
            }
        }

    }
}