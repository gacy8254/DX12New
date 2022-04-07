#include "MainPassCB.hlsli"
#include "ShaderDefinition.h"

#define CENTER_TO_JITTER 0.0f

#define CLIP_TO_CENTER 1

#define USE_CLOSEST_VELOCITY

//#define USE_NEIGHBORHOOD_SPATIAL_WEIGHT
//#define SPATIAL_WEIGHT_CATMULLROM
//#define USE_FIXED_NEIGHBORHOOD_SPATIAL_WEIGHT 1

#define USE_TONEMAP

#define USE_YCOCG

//#define USE_HIGH_PASS_FILTER

//#define USE_FILTER

static const float VarianceClipGamma = 1.0f;
static const float Exposure = 10;
static const float BlendWeightLowerBound = 0.03f;
static const float BlendWeightUpperBound = 0.12f;
static const float BlendWeightVelocityScale = 100.0f * 60.0f;

struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

Texture2D InputTexture : register(t0);
Texture2D HistoryTexture : register(t1);
Texture2D VelocityTexture : register(t2);
Texture2D DepthTexture : register(t3);

struct PixelOutput
{
    float4 current : SV_TARGET0;
    float4 history : SV_TARGET1;
};

SamplerState basicSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);


static const float SampleOffsets[9][2] =
{
    { -1.0f, -1.0f },
    { 0.0f, -1.0f },
    { 1.0f, -1.0f },
    { -1.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { -1.0f, 1.0f },
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
};

static const float neighborhoodFixedSpatialWeight[9] =
{
    1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 4.0f,
	1.0f / 8.0f,
	1.0f / 16.0f,
	1.0f / 8.0f,
	1.0f / 16.0f
};


float3 ClipAABB(float3 _aabbMin, float3 _aabbMax, float3 _previousSample, float3 _avg)
{
#if CLIP_TO_CENTER
    
    float3 p_clip = 0.5 * (_aabbMax + _aabbMin);
    float3 e_clip = 0.5 * (_aabbMax - _aabbMin);
    
    float3 v_clip = _previousSample - p_clip;
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));
    
    if (ma_unit > 1.0)
    {
        return p_clip + v_clip / ma_unit;
    }
    else
    {
        //点在AABB之内
        return _previousSample;
    }
       
#else
    
    float3 r = _previousSample - _avg;
    float3 rmax = _aabbMax - _avg.xyz;
    float3 rmin = _aabbMin - _avg.xyz;

    const float eps = 0.000001f;

    if (r.x > rmax.x + eps)
        r *= (rmax.x / r.x);
    if (r.y > rmax.y + eps)
        r *= (rmax.y / r.y);
    if (r.z > rmax.z + eps)
        r *= (rmax.z / r.z);

    if (r.x < rmin.x - eps)
        r *= (rmin.x / r.x);
    if (r.y < rmin.y - eps)
        r *= (rmin.y / r.y);
    if (r.z < rmin.z - eps)
        r *= (rmin.z / r.z);

    return _avg + r;
    
#endif
}

static float CatmullRom(float x)
{
    float ax = abs(x);
    if (ax > 1.0f)
        return ((-0.5f * ax + 2.5f) * ax - 4.0f) * ax + 2.0f;
    else
        return (1.5f * ax - 2.5f) * ax * ax + 1.0f;
}

//RGB转ycocg
float3 RGB2YCoCgR(float3 rgbColor)
{
    float3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2;

    return YCoCgRColor;
}

//YCOCG转RGB
float3 YCoCgR2RGB(float3 YCoCgRColor)
{
    float3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

float Luminance(in float3 _color)
{
#ifdef USE_TONEMAP
    return _color.r;
    
#else
    return dot(_color, float3(0.25f, 0.50f, 0.25f));
#endif
}

float Luma4(float3 _color)
{
#ifdef USE_TONEMAP
    return _color.r;
    
#else
    return (_color.g * 2.0f) + (_color.r + _color.b);
#endif
}

float3 ToneMap(float3 _color)
{
    return _color / (1 + Luminance(_color));
}

float3 UnToneMap(float3 _color)
{
    return _color / (1 - Luminance(_color));
}

//优化后的HDR加权函数
float HdrWeight4(float3 Color, float Exposure)
{
    return rcp(Luma4(Color) * Exposure + 4.0);
}

//将深度转换为线性
float LinearDepth(float _depth)
{
    return (_depth * NEAR_Z) / (FAR_Z - _depth * (FAR_Z - NEAR_Z));
}

PixelOutput main(PixelShaderInput IN)
{
    PixelOutput output;
    int i, x, y;
    float2 velocity;
    float lenVelocity;
    
    double2 JitteredUV = (double2) IN.TexCoord.xy + double2(gJitter.x, gJitter.y);

    //找出3x3范围内深度最小的像素的速度
#ifdef USE_CLOSEST_VELOCITY
    float2 closestOffset = float2(0, 0);
    float closestDepth = FAR_Z;
    
    for (y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 sampleOffset = float2(x, y) * gInverseRTSize;
            float2 sampleUV = JitteredUV + sampleOffset;
            sampleUV = saturate(sampleUV);
            
            float neighborDepthSamp = DepthTexture.Sample(basicSampler, sampleUV).r;
            neighborDepthSamp = LinearDepth(neighborDepthSamp);
#if USE_REVERSE_Z
            if (neighborDepthSamp > closestDepth)
#else
            if(neighborDepthSamp < closestDepth)
#endif 
            {
                closestDepth = neighborDepthSamp;
                closestOffset = float2(x, y);
            }
        }
    }
    closestOffset *= gInverseRTSize;
    velocity = VelocityTexture.Sample(basicSampler, JitteredUV + closestOffset).rg;
#else
    velocity = VelocityTexture.Sample(basicSampler, JitteredUV).rg;
#endif
    lenVelocity = length(velocity);
    
    float2 InputSamplerUV = lerp(IN.TexCoord.xy, JitteredUV, CENTER_TO_JITTER);
    float3 currentColor = InputTexture.Sample(basicSampler, InputSamplerUV).rgb;
    float3 historytColor = HistoryTexture.Sample(basicSampler, IN.TexCoord - velocity).rgb;
    
#ifdef USE_TONEMAP
    currentColor = ToneMap(currentColor);
    historytColor = ToneMap(historytColor);
#endif
    
#ifdef USE_YCOCG
    currentColor = RGB2YCoCgR(currentColor);
    historytColor = RGB2YCoCgR(historytColor);
#endif
   
    if (gFrameCount == 0)
    {
        output.current = float4(currentColor, 1.0f);
        output.history = float4(currentColor, 1.0f);
        
        return output;
    }
    
#ifdef USE_NEIGHBORHOOD_SPATIAL_WEIGHT
    float neighborhoodSpatialWeight[9];
    float TotalSpatialWeight = 0.0f;
    for (i = 0; i < 9; i++)
    {
        float PixelOffsetX = SampleOffsets[i][0] + gJitter.x;
        float PixelOffsetY = SampleOffsets[i][1] + gJitter.y;

#ifdef SPATIAL_WEIGHT_CATMULLROM
        neighborhoodSpatialWeight[i] = CatmullRom(PixelOffsetX) * CatmullRom(PixelOffsetY);
        TotalSpatialWeight += neighborhoodSpatialWeight[i];
#else
		// Normal distribution, Sigma = 0.47
		neighborhoodSpatialWeight[i] = exp(-2.29f * (PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY));
		TotalSpatialWeight += neighborhoodSpatialWeight[i];
#endif
    }

    for (i = 0; i < 9; i++)
    {
        neighborhoodSpatialWeight[i] = neighborhoodSpatialWeight[i] / TotalSpatialWeight;
    }
#endif
    
    //对临近像素采样
    uint N = 9;
    float TotalWeight = 0.0f;
    float3 sum = 0.0f;
    float3 m1 = 0.0f;
    float3 m2 = 0.0f;
    float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
    float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
    float3 neighborhood[9];
    float neighborhoodHdrWeight[9];
    float neighborhoodFinalWeight = 0.0f;
    
    
    for (y = -1; y <= 1; ++y)
    {
        for (x = -1; x <= 1; ++x)
        {
            i = (y + 1) * 3 + x + 1;
            float2 sampleOffset = float2(x, y) * gInverseRTSize;
            float2 sampleUV = InputSamplerUV + sampleOffset;
            sampleUV = saturate(sampleUV);

            float3 NeighborhoodSamp = InputTexture.Sample(basicSampler, sampleUV).rgb;
            NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
#ifdef USE_TONEMAP
            NeighborhoodSamp = ToneMap(NeighborhoodSamp);
#endif
#ifdef USE_YCOCG
            NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);
#endif

            neighborhood[i] = NeighborhoodSamp;
            neighborhoodHdrWeight[i] = HdrWeight4(NeighborhoodSamp, Exposure);
            neighborMin = min(neighborMin, NeighborhoodSamp);
            neighborMax = max(neighborMax, NeighborhoodSamp);
            
#ifdef USE_NEIGHBORHOOD_SPATIAL_WEIGHT
#if USE_FIXED_NEIGHBORHOOD_SPATIAL_WEIGHT == 1
			neighborhoodFinalWeight = neighborhoodHdrWeight[i] * neighborhoodFixedSpatialWeight[i];
#else
            neighborhoodFinalWeight = neighborhoodHdrWeight[i] * neighborhoodSpatialWeight[i];
#endif
#else 
            neighborhoodFinalWeight = neighborhoodHdrWeight[i];
#endif
            
            m1 += NeighborhoodSamp;
            m2 += NeighborhoodSamp * NeighborhoodSamp;
            TotalWeight += neighborhoodFinalWeight;
            sum += neighborhood[i] * neighborhoodFinalWeight;
        }
    }
    
    float3 Filtered = sum / TotalWeight;
    
#ifdef USE_HIGH_PASS_FILTER
    float3 highFreq = neighborhood[1] + neighborhood[3] + neighborhood[5] + neighborhood[7] - 4 * neighborhood[4];
    float3 sharpen;
    
#ifdef USE_FILTER
    sharpen = Filtered;
#else
    sharpen = currentColor;
#endif
    sharpen += highFreq * 0.1f;
    
#ifdef USE_TONE_MAP   
    saturate(sharpen);
#else
    sharpen = max(sharpen, 0.0f);
#endif
    
#ifdef USE_FILTER      
    Filtered = sharpen;
#else  
    currentColor = sharpen;
#endif
#endif
    
    
    // Variance clip.
    float3 mu = m1 / N;
    float3 sigma = sqrt(abs(m2 / N - mu * mu));
    float3 minc = mu - VarianceClipGamma * sigma;
    float3 maxc = mu + VarianceClipGamma * sigma;
    
    historytColor = ClipAABB(minc, maxc, historytColor, mu);

    float weightCurrent = lerp(BlendWeightLowerBound, BlendWeightUpperBound, saturate(lenVelocity * BlendWeightVelocityScale));
    //weightCurrent = 0.1f;
    float weightPrevious = 1.0f - weightCurrent;
    
    float RcpWeight = rcp(weightCurrent + weightPrevious);
    
#ifdef USE_FILTER      
    float3 color = (Filtered * weightCurrent + weightPrevious * historytColor) * RcpWeight;
#else  
    float3 color = (currentColor * weightCurrent + weightPrevious * historytColor) * RcpWeight;
#endif
    
#ifdef USE_YCOCG
    color = YCoCgR2RGB(color);
#endif
    
#ifdef USE_TONEMAP
    color = UnToneMap(color);
#endif
    

    
    output.current = float4(color, 1.0f);
    output.history = float4(color, 1.0f);

    return output;
}