struct Material
{
    float4 Diffuse;
    //------------------------------------ ( 16 bytes )
    float4 Specular;
    //------------------------------------ ( 16 bytes )
    float4 Emissive;
    //------------------------------------ ( 16 bytes )
    float4 Ambient;
    //------------------------------------ ( 16 bytes )
    float4 Reflectance;
    //------------------------------------ ( 16 bytes )
    float Opacity; // If Opacity < 1, then the material is transparent.
    float SpecularPower;
    float IndexOfRefraction; // For transparent materials, IOR > 0.
    float BumpIntensity; // When using bump textures (height maps) we need
                              // to scale the height values so the normals are visible.
    //------------------------------------ ( 16 bytes )
    bool HasAmbientTexture;
    bool HasEmissiveTexture;
    bool HasDiffuseTexture;
    bool HasSpecularTexture;
    //------------------------------------ ( 16 bytes )
    bool HasSpecularPowerTexture;
    bool HasNormalTexture;
    bool HasBumpTexture;
    bool HasOpacityTexture;
    //------------------------------------ ( 16 bytes )
    // Total:                              ( 16 * 8 = 128 bytes )
};

struct PixelShaderInput
{
    float4 PositionVS : POSITION; //观察空间坐标
    float3 NormalVS : NORMAL; //观察空间法线
    float3 TangentVS : TANGENT; //切线
    float3 BitangentVS : BITANGENT; //副切线
    float2 TexCoord : TEXCOORD; //UV
};

#if ENABLE_LIGHTING
struct PointLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float  Ambient;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    float  QuadraticAttenuation;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct SpotLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionWS; // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS; // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float  Ambient;
    float  SpotAngle;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    //----------------------------------- (16 byte boundary)
    float  QuadraticAttenuation;
    float3 Padding;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 7 = 112 bytes
};

struct DirectionalLight
{
    float4 DirectionWS;  // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS;  // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float Ambient;
    float3 Padding;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct LightProperties
{
    uint NumPointLights;
    uint NumSpotLights;
uint NumDirectionalLights;
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
    float4 Ambient;
};

#endif

ConstantBuffer<Material> MaterialCB : register(b0, space1);

SamplerState TextureSampler : register(s0);

float3 ExpandNormal(float3 _normal)
{
    return _normal * 2.0f - 1.0f;
}

//计算法线贴图
float3 DoNormalMapping(float3x3 _TBN, Texture2D _tex, float2 _uv)
{
    float3 normal = _tex.Sample(TextureSampler, _uv).xyz;
    normal = ExpandNormal(normal);

    //将法线转换到切线空间
    normal = mul(normal, _TBN);
    
    return normalize(normal);
}

//计算凹凸贴图
float3 DoBumpMapping(float3x3 TBN, Texture2D tex, float2 uv, float bumpScale)
{
    // 从高度图中采样当前UV坐标
    float height_00 = tex.Sample(TextureSampler, uv).r * bumpScale;
    // Sample the heightmap in the U texture coordinate direction.
    float height_10 = tex.Sample(TextureSampler, uv, int2(1, 0)).r * bumpScale;
    // Sample the heightmap in the V texture coordinate direction.
    float height_01 = tex.Sample(TextureSampler, uv, int2(0, 1)).r * bumpScale;

    float3 p_00 = { 0, 0, height_00 };
    float3 p_10 = { 1, 0, height_10 };
    float3 p_01 = { 0, 1, height_01 };

    // normal = tangent x bitangent
    float3 tangent = normalize(p_10 - p_00);
    float3 bitangent = normalize(p_01 - p_00);

    float3 normal = cross(tangent, bitangent);

    // Transform normal from tangent space to view space.
    normal = mul(normal, TBN);

    return normal;
}

//如果传入的颜色不是黑色
//将贴图与颜色相乘进行混合
//否则使用贴图颜色替代颜色
float4 SampleTexture(Texture2D _tex, float2 _uv, float4 _color)
{
    if (any(_color.rgb))
    {
        _color *= _tex.Sample(TextureSampler, _uv);
    }
    else
    {
        _color = _tex.Sample(TextureSampler, _uv);
    }

    return _color;
}

//线性到SRGB
float3 LinearToSRGB(float3 _color)
{
    // 准确的SRGB曲线
    //return _color < 0.0031308 ? 12.92 * _color : 1.055 * pow(abs(_color), 1.0 / 2.4) - 0.055;

    //近似的SRGB曲线，但是消耗少
    return _color < 0.0031308 ? 12.92 * _color : 1.13005 * sqrt(abs(_color - 0.00228)) - 0.13448 * _color + 0.005719;
}

//计算漫反射强度
float DoDiffuse(float3 _normal, float3 _light)
{
    return max(0, dot(_normal, _light));
}

//计算高光强度
float DoSpecular(float3 _toView, float3 _normal, float3 _toLight, float _specularPower)
{
    float3 r = normalize(reflect(-_toLight, _normal));
    float rDOTv = max(0, dot(r, _toView));

    return pow(rDOTv, _specularPower);
}

//计算衰减
float DoAttenuation(float _c, float _l, float _q, float _d)
{
    return 1.0f / (_c + _l * _d + _q * _d * _d);
}

//计算射灯夹角
float DoSpotCone(float3 _spotDir, float3 _toLight, float _spotAngle)
{
    float minCos = cos(_spotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(_spotDir, -_toLight);
    
    return smoothstep(minCos, maxCos, cosAngle);
}

#if ENABLE_LIGHTING
ConstantBuffer<LightProperties> LightPropertiesCB : register(b1);

StructuredBuffer<PointLight> PointLights : register(t0);
StructuredBuffer<SpotLight> SpotLights : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);

//计算点光源
LightResult DoPointLight(PointLight _light, float3 _toView, float3 _pos, float3 _normal, float _specularPower)
{
    LightResult result;
    
    float3 toLight = _light.PositionVS.xyz - _pos; //到光源的向量
    float d = length(toLight); //长度
    toLight = toLight / d; //除以长度，归一化
    
    float attenuation = DoAttenuation(_light.ConstantAttenuation, _light.LinearAttenuation, _light.QuadraticAttenuation, d);
    
    result.Diffuse = DoDiffuse(_normal, toLight) * attenuation * _light.Color;
    result.Specular = DoSpecular(_toView, _normal, toLight, _specularPower) * attenuation * _light.Color;
    result.Ambient = _light.Color * _light.Ambient;
    
    return result;
}

//计算射灯
LightResult DoSpotLight(SpotLight _light, float3 _toView, float3 _pos, float3 _normal, float _specularPower)
{
    LightResult result;
    
    float3 toLight = _light.PositionVS.xyz - _pos; //到光源的向量
    float d = length(toLight); //长度
    toLight = toLight / d; //除以长度，归一化
    
    float attenuation = DoAttenuation(_light.ConstantAttenuation, _light.LinearAttenuation, _light.QuadraticAttenuation, d);
    
    float spotIntensity = DoSpotCone(_light.DirectionVS.xyz, toLight, _light.SpotAngle);
    
    result.Diffuse = DoDiffuse(_normal, toLight) * attenuation * spotIntensity * _light.Color;
    result.Specular = DoSpecular(_toView, _normal, toLight, _specularPower) * attenuation * spotIntensity * _light.Color;
    result.Ambient = _light.Color * _light.Ambient;

    return result;
}

//计算平行光
LightResult DoDirectionalLight(DirectionalLight _light, float3 _toView, float3 _pos, float3 _normal, float _specularPower)
{
    LightResult result;
    
    float3 toLight = normalize(-_light.DirectionWS.xyz);
    
    result.Diffuse = _light.Color * DoDiffuse(_normal, toLight);
    result.Specular = DoSpecular(_toView, _normal, toLight, _specularPower) * _light.Color;
    result.Ambient = _light.Color * _light.Ambient;

    return result;
}

LightResult DoLighting(float3 _pos, float3 _normal)
{
    uint i;
    
    float3 V = normalize(-_pos);
    
    LightResult totalResult;
    
    for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
    {
        LightResult result = DoPointLight(PointLights[i], V, _pos, _normal, MaterialCB.SpecularPower);

        totalResult.Specular += result.Specular;
        totalResult.Diffuse += result.Diffuse;
        totalResult.Ambient += result.Ambient;
    }
    
    for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
    {
        LightResult result = DoSpotLight(SpotLights[i], V, _pos, _normal, MaterialCB.SpecularPower);

        totalResult.Specular += result.Specular;
        totalResult.Diffuse += result.Diffuse;
        totalResult.Ambient += result.Ambient;
    }
    
    for (i = 0; i < LightPropertiesCB.NumDirectionalLights; ++i)
    {
        LightResult result = DoDirectionalLight(DirectionalLights[i], V, _pos, _normal, MaterialCB.SpecularPower);

        totalResult.Specular += result.Specular;
        totalResult.Diffuse += result.Diffuse;
        totalResult.Ambient += result.Ambient;
    }

    return totalResult;
}
#endif

Texture2D AmbientTexture : register(t3);
Texture2D EmissiveTexture : register(t4);
Texture2D DiffuseTexture : register(t5);
Texture2D SpecularTexture : register(t6);
Texture2D SpecularPowerTexture : register(t7);
Texture2D NormalTexture : register(t8);
Texture2D BumpTexture : register(t9);
Texture2D OpacityTexture : register(t10);

float4 main(PixelShaderInput IN) : SV_Target
{
    //默认alpha为漫反射贴图的a通道
    //如果有透明度贴图,则从透明度贴图中采样
    float alpha = MaterialCB.Diffuse.a;
    if (MaterialCB.HasOpacityTexture)
    {
        alpha = OpacityTexture.Sample(TextureSampler, IN.TexCoord.xy).r;
    }

#if ENABLE_DECAL
    clip(alpha - 0.1f);
#endif
    
    float4 emissive = MaterialCB.Emissive;
    float4 ambient = MaterialCB.Ambient;
    float4 diffuse = MaterialCB.Diffuse;
    float specularPower = MaterialCB.SpecularPower;
    float2 uv = IN.TexCoord.xy;
    
    if (MaterialCB.HasAmbientTexture)
    {
        ambient = SampleTexture(AmbientTexture, uv, ambient);
    }
    if (MaterialCB.HasEmissiveTexture)
    {
        emissive = SampleTexture(EmissiveTexture, uv, emissive);
    }
    if (MaterialCB.HasDiffuseTexture)
    {
        diffuse = SampleTexture(DiffuseTexture, uv, diffuse);
    }
    if (MaterialCB.HasSpecularPowerTexture)
    {
        specularPower *= SpecularPowerTexture.Sample(TextureSampler, uv).r;
    }
    
    float3 N;
    if (MaterialCB.HasNormalTexture)
    {
        float3 tangent = normalize(IN.TangentVS);
        float3 bittangent = normalize(IN.BitangentVS);
        float3 normal = normalize(IN.NormalVS);
        
        float3x3 TBN = float3x3(tangent, bittangent, normal);
        
        N = DoNormalMapping(TBN, NormalTexture, uv);
    }
    else if (MaterialCB.HasBumpTexture)
    {
        float3 tangent = normalize(IN.TangentVS);
        float3 bittangent = normalize(IN.BitangentVS);
        float3 normal = normalize(IN.NormalVS);
        
        float3x3 TBN = float3x3(tangent, -bittangent, normal);
        
        N = DoBumpMapping(TBN, BumpTexture, uv, MaterialCB.BumpIntensity);
    }
    else
    {
        N = normalize(IN.NormalVS);
    }
    
    
    float shadow = 1;
    float4 specular = 0;
#if ENABLE_LIGHTING
    LightResult lit = DoLighting(IN.PositionVS.xyz, normalize(IN.NormalVS));
    diffuse *= lit.Diffuse;
    ambient *= lit.Ambient;
    
    if(MaterialCB.SpecularPower > 1.0f)
    {
        specular = MaterialCB.Specular;
        if(MaterialCB.HasSpecularTexture)
        {
            specular = SampleTexture(SpecularTexture, uv, specular);
        }
        specular *= lit.Specular;
    }
#else
    shadow = -N.z;
#endif
    
    
    return float4((emissive + ambient + diffuse + specular).rgb * shadow, alpha * MaterialCB.Opacity);
}
