#include "ShaderDefinition.h"
#include "Lighting.hlsli"
#include "MainPassCB.hlsli"

RWStructuredBuffer<LightList> gLightList : register(u0);

//��ȷ�Ƭ����
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
    //�����߳������̵߳����,����һ��һά��ֵ
    uint groupIndex = groupThreadID.y * CLUSTER_THREAD_NUM_X + groupThreadID.x;
    
    //�����߳���ȫ�ַ�Χ�ڵĶ�ά����
    uint2 globalCoords = dispatchThreadID.xy;
    
    //����ִص���ʼλ��
    uint clusterStart = (groupID.y * ceil(gRTSize.x / CLUSTER_SIZE_X) + groupID.x) * CLUSTER_NUM_Z;
    
    //�Եƹ��б��ʼ��
    if (groupIndex < CLUSTER_NUM_Z)
    {
        gLightList[clusterStart + groupIndex].NumPointLights = 0;
        gLightList[clusterStart + groupIndex].NumSpotLights = 0;
    }
    
    //�ȴ������߳���ɼ���
    GroupMemoryBarrierWithGroupSync();
    
    //����� ����/ƫ�� ��[0, 1]
    //������Ļ��Ƭ������ʹ��RT�Ĵ�С���Էִص�����
    float2 tileNum = float2(gRTSize.xy) * rcp(float2(CLUSTER_SIZE_X, CLUSTER_SIZE_Y));
    //���㵱ǰ��Ƭ�����ĵ�ƫ����
    float2 tileCenterOffset = float2(groupID.xy) * 2 + float2(1.0f, 1.0f) - tileNum;
    
    //����ͶӰ����
    float4 c1 = float4(-gProj._11 * tileNum.x, 0.0f, tileCenterOffset.x, 0.0f);
    float4 c2 = float4(0.0f, -gProj._22 * tileNum.y, -tileCenterOffset.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);
    
    //������׵���������
    float4 frustumPlanes[6];

    //����
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;
    
    //��һ��
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }

    //�ӵ�ǰ�ִ����޳��ƹ�
    for (uint lightIndex = groupIndex; lightIndex < (uint) LightPropertiesCB.NumPointLights; lightIndex += COMPUTE_SHADER_CLUSTER_GROUP_SIZE)
    {
        bool inFrustum = true;
        
        //����ƹ��ڹ۲�ռ��е�����
        float4 lightPositionView = mul(gView, float4(PointLights[lightIndex].PositionWS.xyz, 1.0f));

        [unroll]
        for (uint i = 0; i < 4; i++)
        {
            //�жϵƹ��Ƿ���Ӱ�쵽��ǰ�ִ�(��������)
            float d = dot(frustumPlanes[i], lightPositionView);
            inFrustum = inFrustum && (d >= -PointLights[lightIndex].range);
        }
        
        if (inFrustum)
        {
            [unroll]
            for (uint clusterZ = 0; clusterZ < CLUSTER_NUM_Z; clusterZ++)
            {
                //��׶��ǰ����
                inFrustum = true;
                frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -DepthSlicing_64[clusterZ]);
                frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, DepthSlicing_64[clusterZ + 1]);
                
                //�жϵƹ��Ƿ���Ӱ�쵽��ǰ�ִ�(ǰ��)
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