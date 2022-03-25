#define BLOCK_SIZE 16

struct CSInput
{
    uint3 GroupID : SV_GroupID; //线程组索引        
    uint3 GroupThreadID : SV_GroupThreadID; //组内的线程索引
    uint3 DispatchThreadID : SV_DispatchThreadID; //全局的线程索引
    uint GroupIndex : SV_GroupIndex; //线程的索引
};

struct PanoToCubemap
{
    uint CubemapSize;
    uint FirstMip;
    uint NumMips;
};

ConstantBuffer<PanoToCubemap> PanoToCubemapCB : register(b0);

Texture2D<float4> SrcTexture : register(t0);

RWTexture2DArray<float4> DstMip1 : register(u0);
RWTexture2DArray<float4> DstMip2 : register(u1);
RWTexture2DArray<float4> DstMip3 : register(u2);
RWTexture2DArray<float4> DstMip4 : register(u3);
RWTexture2DArray<float4> DstMip5 : register(u4);

SamplerState LinearRepeatSampler : register(s0);

#define GenerateMips_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 3), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 5) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT )"

// 1 / PI
static const float InvPI = 0.31830988618379067153776752674503f;
static const float Inv2PI = 0.15915494309189533576888376337251f;
static const float2 InvAtan = float2(Inv2PI, InvPI);

// 转换到立方体贴图每个面的朝向
static const float3x3 RotateUV[6] =
{
    // +X
    float3x3(0, 0, 1,
               0, -1, 0,
              -1, 0, 0),
    // -X
    float3x3(0, 0, -1,
               0, -1, 0,
               1, 0, 0),
    // +Y
    float3x3(1, 0, 0,
               0, 0, 1,
               0, 1, 0),
    // -Y
    float3x3(1, 0, 0,
               0, 0, -1,
               0, -1, 0),
    // +Z
    float3x3(1, 0, 0,
               0, -1, 0,
               0, 0, 1),
    // -Z
    float3x3(-1, 0, 0,
               0, -1, 0,
               0, 0, -1)
};


[RootSignature(GenerateMips_RootSignature)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CSInput IN)
{
    //立方体贴图UV
    uint3 texCoord = IN.DispatchThreadID;
    
    //首先检查线程是否符合立方体贴图的维度
    if (texCoord.x >= PanoToCubemapCB.CubemapSize || texCoord.y >= PanoToCubemapCB.CubemapSize)
        return;
    
    //将立方体贴图的UV坐标映射成一个向量
    // [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
    float3 dir = float3(texCoord.xy / float(PanoToCubemapCB.CubemapSize) - 0.5f, 0.5f);
    
    //旋转到对应的立方体贴图的面
    dir = normalize(mul(RotateUV[texCoord.z], dir));
    
    //将世界空间中的方向转变为全景图中的贴图坐标
    float2 panoUV = float2(atan2(-dir.x, -dir.z), acos(dir.y)) * InvAtan;
    
    DstMip1[texCoord] = SrcTexture.SampleLevel(LinearRepeatSampler, panoUV, PanoToCubemapCB.FirstMip);
    
    // 仅在2的倍数的线程上执行
    if (PanoToCubemapCB.NumMips > 1 && (IN.GroupIndex & 0x11) == 0)
    {
        DstMip2[uint3(texCoord.xy / 2, texCoord.z)] = SrcTexture.SampleLevel(LinearRepeatSampler, panoUV, PanoToCubemapCB.FirstMip + 1);
    }

    // 仅在4的倍数的线程上执行
    if (PanoToCubemapCB.NumMips > 2 && (IN.GroupIndex & 0x33) == 0)
    {
        DstMip3[uint3(texCoord.xy / 4, texCoord.z)] = SrcTexture.SampleLevel(LinearRepeatSampler, panoUV, PanoToCubemapCB.FirstMip + 2);
    }

    //仅在8的倍数的线程上执行
    if (PanoToCubemapCB.NumMips > 3 && (IN.GroupIndex & 0x77) == 0)
    {
        DstMip4[uint3(texCoord.xy / 8, texCoord.z)] = SrcTexture.SampleLevel(LinearRepeatSampler, panoUV, PanoToCubemapCB.FirstMip + 3);
    }

    // 仅在16的倍数的线程上执行
    if (PanoToCubemapCB.NumMips > 4 && (IN.GroupIndex & 0xFF) == 0)
    {
        DstMip5[uint3(texCoord.xy / 16, texCoord.z)] = SrcTexture.SampleLevel(LinearRepeatSampler, panoUV, PanoToCubemapCB.FirstMip + 4);
    }
}