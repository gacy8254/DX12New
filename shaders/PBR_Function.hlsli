#include "CommonTool.hlsli"

//菲涅尔方程
//返回物体表面光线被反射的百分比
float3 FresnelSchlick(float _conTheta, float3 _F0)
{
    return _F0 + (1.0f - _F0) * pow(clamp(1.0f - _conTheta, 0.0f, 1.0f), 5.0f);
}

//计算菲涅尔项
float3 CalFresnel(float3 _halfVec, float3 _toCam, float3 _albedo, float _metallic)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, _albedo, _metallic);
    
    float3 f = FresnelSchlick(clamp(dot(_halfVec, _toCam), 0.0f, 1.0f), F0);
    
    return f;
}

//带粗糙度的菲涅尔方程
float3 FresnelSchlickRoughness(float _conTheta, float3 _F0, float _roughness)
{
    return _F0 + (max((float3) (1.0f - _roughness), _F0) - _F0) * pow(clamp(1.0f - _conTheta, 0.0f, 1.0f), 5.0f);
}

//计算菲涅尔项
float3 CalFresnelRoughness(float3 _halfVec, float3 _toCam, float3 _albedo, float _metallic, float _roughness)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, _albedo.rgb, _metallic);
    
    float3 f = FresnelSchlickRoughness(clamp(dot(_halfVec, _toCam), 0.0f, 1.0f), F0, _roughness);
    
    return f;
}

//正态分布函数
float DistributionGGX(float3 _normal, float3 _halfVec, float _roughness)
{
    //_roughness += 0.0001f;
    float a = _roughness * _roughness;
    float a2 = a * a;
    float NdotH = max(dot(_normal, _halfVec), 0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float _NdotV, float _roughness, bool _ibl = false)
{
    float r = (_roughness + 1.0f);
    float k;
    
    if (_ibl)
    {
        k = (_roughness * _roughness) / 2.0f;
    }
    else
    {
        k = (r * r) / 8.0f;
    }
     
    
    float nom = _NdotV;
    float denom = _NdotV * (1.0f - k) + k;
    
    return nom / denom;
}

//几何遮蔽
float GeometrySmith(float3 _normal, float3 _toCam, float3 _toLight, float _roughness, bool _ibl = false)
{
    float NdotV = max(dot(_normal, _toCam), 0.0f);
    float NdotL = max(dot(_normal, _toLight), 0.0f);
    
    float ggx2 = GeometrySchlickGGX(NdotV, _roughness, _ibl);
    float ggx1 = GeometrySchlickGGX(NdotL, _roughness, _ibl);
    
    return ggx2 * ggx1;
}

//低差异序列
float RadicalInverse_VdC(uint _bits)
{
   _bits = (_bits << 16u) | (_bits >> 16u);
   _bits = ((_bits & 0x55555555u) << 1u) | ((_bits & 0xAAAAAAAAu) >> 1u);
   _bits = ((_bits & 0x33333333u) << 2u) | ((_bits & 0xCCCCCCCCu) >> 2u);
   _bits = ((_bits & 0x0F0F0F0Fu) << 4u) | ((_bits & 0xF0F0F0F0u) >> 4u);
   _bits = ((_bits & 0x00FF00FFu) << 8u) | ((_bits & 0xFF00FF00u) >> 8u);
    return float(_bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley 序列
float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

//重要性采样
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{

    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

//BRDF卷积
float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    float3 N = float3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float G;
        if (NdotL > 0.0)
        {
            G = GeometrySmith(N, V, L, roughness, true);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
           
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return float2(A, B);
}