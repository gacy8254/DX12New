
//����X Yά���е��߳����С
#define BLOCK_SIZE 8

#define WIDTH_HEIGHT_EVEN 0             //��߶���ż��
#define WIDTH_ODD_HEIGHT_EVEN 1         //��������ż��
#define WIDTH_EVEN_HEIGHT_ODD 2         //��ż��������
#define WIDTH_HEIGHT_ODD 3              //��߶�������

struct CSInput
{
    uint3 GroupID : SV_GroupID;                    //�߳�������        
    uint3 GroupThreadID : SV_GroupThreadID;        //���ڵ��߳�����
    uint3 DispatchThreadID : SV_DispatchThreadID;  //ȫ�ֵ��߳�����
    uint GroupIndex : SV_GroupIndex;               //�̵߳�����
};

cbuffer GenerateMipCB : register(b0)
{
    uint SrcMipLevel;  //��ͼԭ����Mip�ȼ�
    uint NumMiplevels; //���ɶ��ٸ�MIPS һ�����4��
    uint SrcDimension; //ȷ��Դmip�Ŀ���Ƿ�������
    bool IsSRGB;       //�Ƿ���SRGB
    float2 TextlSize; //����Ŀ�� mip �е������أ�ʹ�ù�һ���������꣩��ƫ������������������ά�ȵ�����¼�������������������ƫ��
}

//�����mipmap
Texture2D<float4> SrcMip : register(t0);

//�����Mip
RWTexture2D<float4> outMip1 : register(u0);
RWTexture2D<float4> outMip2 : register(u1);
RWTexture2D<float4> outMip3 : register(u2);
RWTexture2D<float4> outMip4 : register(u3);

//����ǯ�Ʋ�����
SamplerState LinearClampSampler : register(s0);

//�ɱ����ɫ���ĸ�ǩ������ֱ�����ַ���������ɫ����ָ��
#define GenerateMips_RootSignature                  \
"RootFlags(0),"                                      \
"RootConstants(b0, num32BitConstants = 6),"         \
"DescriptorTable(SRV(t0, numDescriptors = 1)),"     \
"DescriptorTable(UAV(u0, numDescriptors = 4)),"     \
"StaticSampler(s0,"                                 \
"addressU = TEXTURE_ADDRESS_CLAMP,"                 \
"addressV = TEXTURE_ADDRESS_CLAMP,"                 \
"addressW = TEXTURE_ADDRESS_CLAMP,"                 \
"filter = FILTER_MIN_MAG_MIP_LINEAR)"  



groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];
groupshared float gs_A[64];


void StoreColor(uint _index, float4 _color)
{
    //����ɫ��ֵ��ĸ������ڴ���
    gs_R[_index] = _color.r;
    gs_G[_index] = _color.g;
    gs_B[_index] = _color.b;
    gs_A[_index] = _color.a;
}

//���ĸ���ɫ�������л��
float4 LoadColor(uint _index)
{
    return float4(gs_R[_index], gs_G[_index], gs_B[_index], gs_A[_index]);
}

//ת����������ɫ�ռ�
float3 ConvertToLinear(float3 _x)
{
    return _x < 0.04045f ?  _x / 12.92 : pow((_x + 0.055) / 1.055, 2.4f);
}

//ת����SRGB
float3 ConvertToSRGB(float3 _x)
{
    return _x < 0.0031308 ? 12.92 * _x : 1.055 * pow(abs(_x), 1.0 / 2.4) - 0.055;
}

float4 PackColor(float4 _x)
{
    if(IsSRGB)
    {
        return float4(ConvertToSRGB(_x.rgb), _x.a);
    }
    else
    {
        return _x;
    }
}

[RootSignature(GenerateMips_RootSignature)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main( CSInput IN)
{
    float4 src1 = (float4)0;
    
    switch (SrcDimension)
    {
        case WIDTH_HEIGHT_EVEN:
        {
                float2 uv = TextlSize * (IN.DispatchThreadID.xy + 0.5);
                src1 = SrcMip.SampleLevel(LinearClampSampler, uv, SrcMipLevel);
        }
            break;
        case WIDTH_ODD_HEIGHT_EVEN:
        {
                float2 uv1 = TextlSize * (IN.DispatchThreadID.xy + float2(0.25, 0.5));
                float2 off = TextlSize * float2(0.5, 0.0);
                src1 = 0.5f * (SrcMip.SampleLevel(LinearClampSampler, uv1, SrcMipLevel) + 
                                SrcMip.SampleLevel(LinearClampSampler, uv1 + off, SrcMipLevel));
            }
            break;
        case WIDTH_EVEN_HEIGHT_ODD:
        {
                float2 uv1 = TextlSize * (IN.DispatchThreadID.xy + float2(0.5, 0.25));
                float2 off = TextlSize * float2(0.0, 0.5);
                src1 = 0.5f * (SrcMip.SampleLevel(LinearClampSampler, uv1, SrcMipLevel) +
                                SrcMip.SampleLevel(LinearClampSampler, uv1 + off, SrcMipLevel));
            }
            break;
        case WIDTH_HEIGHT_ODD:
        {
                float2 uv1 = TextlSize * (IN.DispatchThreadID.xy + float2(0.25, 0.25));
                float2 off = TextlSize * 0.5f;
                src1 = SrcMip.SampleLevel(LinearClampSampler, uv1, SrcMipLevel);
                src1 += SrcMip.SampleLevel(LinearClampSampler, uv1 + float2(off.x, 0.0), SrcMipLevel);
                src1 += SrcMip.SampleLevel(LinearClampSampler, uv1 + float2(0.0, off.y), SrcMipLevel);
                src1 += SrcMip.SampleLevel(LinearClampSampler, uv1 + float2(off.x, off.y), SrcMipLevel);
                src1 *= 0.25f;

            }
            break;
    }
    
    //���ͼƬ��SRGB
    //����ɫ����SRGB�ռ�
    outMip1[IN.DispatchThreadID.xy] = PackColor(src1);
    
    //���ֻ����һ��Mipmap���Ѿ����
    if (NumMiplevels == 1)
        return;
    
    //����ͼ����д�빲���ڴ�
    StoreColor(IN.GroupIndex, src1);
    
    //ȷ�������̶߳��Ѿ����
    GroupMemoryBarrierWithGroupSync();
    
    //���ɵڶ���Mipʱֻ��Ҫ�ķ�֮һ���̲߳���
    //����������ŵ��߳�
    if ((IN.GroupIndex & 0x9) == 0)
    {
        float4 src2 = LoadColor(IN.GroupIndex + 0x01);
        float4 src3 = LoadColor(IN.GroupIndex + 0x08);
        float4 src4 = LoadColor(IN.GroupIndex + 0x09);
        
        src1 = 0.25 * (src1 + src2 + src3 + src4);
        
        outMip2[IN.DispatchThreadID.xy / 2] = PackColor(src1);
        StoreColor(IN.GroupIndex, src1);
    }

    if (NumMiplevels == 2)
        return;
    
    //ȷ�������̶߳��Ѿ����
    GroupMemoryBarrierWithGroupSync();
    
      //���ɵ�����Mipʱֻ��Ҫʮ����֮һ���̲߳���
    //����������ŵ��߳�
    if ((IN.GroupIndex & 0x1B) == 0)
    {
        float4 src2 = LoadColor(IN.GroupIndex + 0x02);
        float4 src3 = LoadColor(IN.GroupIndex + 0x10);
        float4 src4 = LoadColor(IN.GroupIndex + 0x12);
        
        src1 = 0.25 * (src1 + src2 + src3 + src4);
        
        outMip3[IN.DispatchThreadID.xy / 4] = PackColor(src1);
        StoreColor(IN.GroupIndex, src1);
    }

    if (NumMiplevels == 3)
        return;
    
    //ȷ�������̶߳��Ѿ����
    GroupMemoryBarrierWithGroupSync();
    
     //���ɵ�����Mipʱֻ��Ҫ�߳����еĵ�һ���߳���Ҫ����
    //����������ŵ��߳�
    if (IN.GroupIndex == 0)
    {
        float4 src2 = LoadColor(IN.GroupIndex + 0x04);
        float4 src3 = LoadColor(IN.GroupIndex + 0x20);
        float4 src4 = LoadColor(IN.GroupIndex + 0x24);
        
        src1 = 0.25 * (src1 + src2 + src3 + src4);
        
        outMip3[IN.DispatchThreadID.xy / 8] = PackColor(src1);
        StoreColor(IN.GroupIndex, src1);
    }
}