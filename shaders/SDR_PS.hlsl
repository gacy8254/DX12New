//映射方法
#define TM_Linear     0
#define TM_Reinhard   1
#define TM_ReinhardSq 2
#define TM_ACESFilmic 3

struct TonemapParameters
{
    //映射方法
    uint TonemapMethed;
    //曝光度
    float Exposure;
    //最大流明，用于线性映射
    float MaxLuminance;
    //莱因哈德常数，一般为1.0
    float K;
    
    //ACES Filmic参数
    float A; // Shoulder strength
    float B; // Linear strength
    float C; // Linear angle
    float D; // Toe strength
    float E; // Toe Numerator
    float F; // Toe denominator
    //E/F = ToeAngle
    float LinearWhite;
    
    float Gamma;
};

ConstantBuffer<TonemapParameters> TonemapParametersCB : register(b0);

Texture2D<float3> HDRTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

//线性映射
float3 Linear(float3 _hdr, float _max)
{
    float3 SDR = _hdr;
    if(_max > 0)
    {
        SDR = saturate(_hdr / _max);
    }
    
    return SDR;
}

float3 Reinhard(float3 _hdr , float _k)
{
    return _hdr / (_hdr + _k);
}

float3 ReinhardSqr(float3 _hdr, float _k)
{
    return pow(Reinhard(_hdr, _k), 2);
}

float3 ACESFilmic(float3 _x, float _a, float _b, float _c, float _d, float _e, float _f)
{
    return ((_x * (_a * _x + _c * _b) + _d * _e) / (_x * (_a * _x + _b) + _d * _f)) - (_e / _f);
}

float4 main(float2 TexCoord : TEXCOORD) : SV_TARGET0
{
    float3 HDR = HDRTexture.SampleLevel(LinearClampSampler, TexCoord, 0);
    
    //根据曝光度调整结果
   HDR *= exp2(TonemapParametersCB.Exposure);
    
    float3 sdr = HDR;
    
    switch (TonemapParametersCB.TonemapMethed)
    {
        case TM_Linear:
            sdr = Linear(HDR, TonemapParametersCB.MaxLuminance);
            break;
        case TM_Reinhard:
            sdr = Reinhard(HDR, TonemapParametersCB.K);
            break;
        case TM_ReinhardSq:
            sdr = ReinhardSqr(HDR, TonemapParametersCB.K);
            break;
        case TM_ACESFilmic:
            sdr = ACESFilmic(HDR, TonemapParametersCB.A, TonemapParametersCB.B, TonemapParametersCB.C, TonemapParametersCB.D, TonemapParametersCB.E, TonemapParametersCB.F) /
        ACESFilmic(TonemapParametersCB.LinearWhite, TonemapParametersCB.A, TonemapParametersCB.B, TonemapParametersCB.C, TonemapParametersCB.D, TonemapParametersCB.E, TonemapParametersCB.F);
            break;
    }
    
    return float4(pow(abs(sdr), 1.0f / TonemapParametersCB.Gamma), 1);
    //return float4(sdr, 1.0f);

}