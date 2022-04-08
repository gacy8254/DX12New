#include "ShaderDefinition.h"
//#if ENABLE_LIGHTING
struct PointLight
{
    float4 PositionWS; // Light position in world space.
    //----------------------------------- (16 byte boundary)
    float4 PositionVS; // Light position in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float range;
    float Intensity;
    float LinearAttenuation;
    float QuadraticAttenuation;
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
    float range;
    float SpotAngle;
    float Intensity;
    float LinearAttenuation;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 6 = 96 bytes
};

struct DirectionalLight
{
    float4 DirectionWS; // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    float4 DirectionVS; // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    float4 Color;
    //----------------------------------- (16 byte boundary)
    float Intensity;
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

struct LightList
{
    uint PointlightIndices[MAX_GRID_POINT_LIGHT_NUM];
    uint NumPointLights;
    uint SpotlightIndices[MAX_GRID_SPOTLIGHT_NUM];
    uint NumSpotLights;
};

ConstantBuffer<LightProperties> LightPropertiesCB           : register(b0);

StructuredBuffer<PointLight> PointLights                    : register(t0);
StructuredBuffer<SpotLight> SpotLights                      : register(t1);
StructuredBuffer<DirectionalLight> DirectionalLights        : register(t2);
StructuredBuffer<LightList> LightsList                      : register(t3);