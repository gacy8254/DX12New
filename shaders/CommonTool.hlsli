



struct Material
{
    float4 Emissive;
    //----------------------------------- (16 byte boundary)
    float4 Ambient;
    //----------------------------------- (16 byte boundary)
    float4 Diffuse;
    //----------------------------------- (16 byte boundary)
    float4 Specular;
    //----------------------------------- (16 byte boundary)
    float SpecularPower;
    float3 Padding;//占位
    //----------------------------------- (16 byte boundary)
    //Total                               (16 * 5 = 80 bytes)
};

struct PointLight
{
    float4 PositionWS;
    //----------------------------------- (16 byte boundary)
    float4 PositionVS;
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float ConstantAttenuation;            //衰减常量
    float LinearAttenuation;              //衰减一次项      
    float QuadraticAttenuation;           //衰减二次项
    float Padding;                        //占位
    //----------------------------------- (16 byte boundary)
     //Total                               (16 * 4 = 64 bytes)
};

struct SpotLight
{
    float4 PositionWS;
    //----------------------------------- (16 byte boundary)
    float4 PositionVS;
    //----------------------------------- (16 byte boundary)
    float4 DirectionWS; // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS; // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float SpotAngle;
    float ConstantAttenuation; //衰减常量
    float LinearAttenuation; //衰减一次项      
    float QuadraticAttenuation; //衰减二次项
    //----------------------------------- (16 byte boundary)
     //Total                               (16 * 4 = 64 bytes)
};

struct LightProperties
{
    uint NumPointLights;
    uint NumSpotLights;
};

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
};

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
float DoSpotCone(float3 _spotDir, float _toLight, float _spotAngle)
{
    float minCos = cos(_spotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(_spotDir, -_toLight);
    
    return smoothstep(minCos, maxCos, cosAngle);
}

//计算点光源
LightResult DoPointLight(PointLight _light, float3 _toView, float3 _pos, float3 _normal, float _specularPower)
{
    LightResult result;
    
    float3 toLight = _light.PositionVS.xyz - _pos;//到光源的向量
    float d = length(toLight);//长度
    toLight = toLight / d;//除以长度，归一化
    
    float attenuation = DoAttenuation(_light.ConstantAttenuation, _light.LinearAttenuation, _light.QuadraticAttenuation, d);
    
    result.Diffuse = DoDiffuse(_normal, toLight) * attenuation * _light.Color;
    result.Specular = DoSpecular(_toView, _normal, toLight, _specularPower) * attenuation * _light.Color;
    
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

    return result;
}

