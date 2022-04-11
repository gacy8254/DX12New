#include "Lighting.hlsli"

//菲涅尔方程
//返回物体表面光线被反射的百分比
float3 FresnelSchlick(float3 _halfVec, float3 _toCam, float3 _F0)
{
    float cosTheta = clamp(dot(_halfVec, _toCam), 0.0f, 1.0f);
    return _F0 + (1.0f - _F0) * exp2((-5.55473 * cosTheta - 6.98316) * cosTheta);
   // return _F0 + (1.0f - _F0) * pow(clamp(1.0f - conTheta, 0.0f, 1.0f), 5.0f);
}

//带粗糙度的菲涅尔方程
float3 FresnelSchlickRoughness(float3 _halfVec, float3 _toCam, float3 _F0, float _roughness)
{
    float conTheta = clamp(dot(_halfVec, _toCam), 0.0f, 1.0f);
    return _F0 + (max((float3) (1.0f - _roughness), _F0) - _F0) * pow(clamp(1.0f - conTheta, 0.0f, 1.0f), 5.0f);
}

//正态分布函数
float DistributionGGX(float3 _normal, float3 _halfVec, float _roughness)
{
    _roughness = max(0.01f, _roughness);
    float a = _roughness * _roughness;
    float a2 = a * a;
    float NdotH = max(dot(_normal, _halfVec), 0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f) + 0.0000001f;
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

//计算衰减
float DoAttenuation(float3 _position, float _range, float3 _worldPos)
{
#if UNREAL_LIGHT_ATTENUATION
    
    float dist = distance(_position, _worldPos);
    float numer = dist / _range;
    numer = numer * numer;
    numer = numer * numer;
    numer = saturate(1 - numer);
    numer = numer * numer;
    float demon = dist * dist  + 1;
    return numer / demon;
    
#else
    
    float dist = distance(_position, _worldPos);
    float att = saturate(1.0f - (dist * dist) / (_range * _range));
    
    return att;
    
#endif
}

//射灯衰减
float DoSpotCone(float3 spotDir, float3 L, float spotAngle)
{
    float minCos = cos(spotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(spotDir, -L);
    return smoothstep(minCos, maxCos, cosAngle);
    //return cosAngle;
}

float3 CookTorrance(float3 _normal, float3 _toLight, float3 _toCamera, float _roughness, float _metallic, float3 _f0, out float3 _ks)
{
    float3 result = (float3) 0.0f;
    
    float3 halfVec = normalize(_toCamera + _toLight);
    
    float D = DistributionGGX(_normal, halfVec, _roughness);
    float F = FresnelSchlick(halfVec, _toCamera, _f0);
    float G = GeometrySmith(_normal, _toCamera, _toLight, _roughness, false);
    
    _ks = F;
    
    float3 numerator = D * G * F;
    float demoninator = 4.0 * max(dot(_normal, _toCamera), 0.0f) * max(dot(_normal, _toLight), 0.0f) + 0.0001f;
    
    float3 specular = numerator / demoninator;
    
    return specular;
}

float3 LamberDiffuse(float3 _ks, float3 _albedo, float _metallic)
{
    float3 KD = (float3) 1.0f - _ks;
    KD *= 1.0f - _metallic;
    
    return (KD * _albedo / PI);
}

//计算直接光照
float3 DirectPBR(float _lightInstensity, float3 _lightColor,
float3 _toLight, float3 _normal,
float3 _worldPos, float3 _toCamera,
float3 _albedo, float _roughness,
float _metallic, float3 _f0,
float _shadowAmount = 1.0f)
{
    float3 result;
    
    float3 KS = float3(0.0f, 0.0f, 0.0f);
    
    float3 specularBRDF = CookTorrance(_normal, _toLight, _toCamera, _roughness, _metallic, _f0, KS);
    float3 diffuseBRDF = LamberDiffuse(KS, _albedo, _metallic);
    
    float NdotL = max(dot(_normal, _toLight), 0.0);
    
    result = (specularBRDF + diffuseBRDF) * NdotL * _lightInstensity * _lightColor;

    return result;
}

float3 DoPointLight(PointLight light, float3 _normal, float3 _worldPos, float3 _toCamera, float3 _albedo, float _roughness, float _metallic, float3 _f0)
{
    float3 result;
    
    float3 toLight = normalize(light.PositionWS.xyz - _worldPos);

    float att = DoAttenuation(light.PositionWS.xyz, light.range, _worldPos);
    float instensity = att * light.Intensity;
    
    result = DirectPBR(instensity, light.Color.rgb, toLight, _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, _f0);

    return result;
}

float3 DoSpotLight(SpotLight light, float3 _normal, float3 _worldPos, float3 _toCamera, float3 _albedo, float _roughness, float _metallic, float3 _f0)
{
    float3 result;
    float3 toLight = normalize(light.PositionWS.xyz - _worldPos);
    
    float spotAtt = DoSpotCone(light.DirectionWS.xyz, toLight, light.SpotAngle);
    float att = DoAttenuation(light.PositionWS.xyz, light.range, _worldPos);
   
    float instensity = att * light.Intensity * spotAtt;
    
    result = DirectPBR(instensity, light.Color.rgb, toLight, _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, _f0);
    return result;
}

float3 DoDirectionalLight(DirectionalLight light, float3 _normal, float3 _worldPos, float3 _toCamera, float3 _albedo, float _roughness, float _metallic, float3 _f0)
{
    float3 result = (float3) 0;
    
    float3 toLight = normalize(light.DirectionWS.xyz);
    float instensity = light.Intensity;
    
    result = DirectPBR(instensity, light.Color.rgb, toLight, _normal, _worldPos, _toCamera, _albedo, _roughness, _metallic, _f0);

    return result;
}

//计算环境光照
//分别返回环境漫反射光照和镜面反射光照
void AmbientPBR(float3 _normal, float3 _worldPos, 
float3 _toCamera, float3 _albedo, 
float _roughness, float _metallic, 
float3 _irradiance, float3 _prefilteredColor, 
float2 _brdf, float _shadowAmount, float2 _aoRO,
inout float3 _ambientDiffuse, inout float3 _ambientSpecular)
{
    float3 totalResult = (float3) 0.0f;
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, _albedo.rgb, _metallic);
    
    float3 KS = FresnelSchlickRoughness(_normal, _toCamera, F0, _roughness);
    
    float3 kD = float3(1.0f, 1.0f, 1.0f) - KS;
    kD *= (1.0f - _metallic);
    
    float3 specular = _prefilteredColor * (KS * _brdf.x + _brdf.y);
    float3 diffuse = _irradiance * _albedo;
    
    _ambientDiffuse = kD * diffuse * _aoRO.r;
    _ambientSpecular = specular * _aoRO.r;
}