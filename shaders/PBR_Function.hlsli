#define PI 3.141592653589793238462643383279

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

float GeometrySchlickGGX(float _NdotV, float _roughness)
{
    float r = (_roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float nom = _NdotV;
    float denom = _NdotV * (1.0f - k) + k;
    
    return nom / denom;
}

//几何遮蔽
float GeometrySmith(float3 _normal, float3 _toCam, float3 _toLight, float _roughness)
{
    float NdotV = max(dot(_normal, _toCam), 0.0f);
    float NdotL = max(dot(_normal, _toLight), 0.0f);
    
    float ggx2 = GeometrySchlickGGX(NdotV, _roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, _roughness);
    
    return ggx2 * ggx1;
}