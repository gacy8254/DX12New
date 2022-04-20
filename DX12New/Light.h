#pragma once
#include "Matrix.h"
/*
 *  Copyright(c) 2020 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

 /**
  *  @file Light.h
  *  @date November 10, 2020
  *  @author Jeremiah van Oosten
  *
  *  @brief Light structures that use HLSL constant buffer padding rules.
  */

#include <DirectXMath.h>

struct PointLight
{
    PointLight()
        :PositionWS(0.0f, 0.0f, 0.0f, 1.0f)
        , PositionVS(0.0f, 0.0f, 0.0f, 1.0f)
        , Color(1.0f, 1.0f, 1.0f, 1.0f)
        , Range(0.01f)
        , Instensity(1.0f)
        , ShadowMapIndex(0)
        , HasShadowMap(false)
    {}
   
    DirectX::XMFLOAT4 PositionWS;  // Light position in world space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 PositionVS;  // Light position in view space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Color;
    //----------------------------------- (16 byte boundary)
    float Range;
    float Instensity;
    unsigned int ShadowMapIndex;
    bool HasShadowMap;
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 4 = 64 bytes
};

struct SpotLight
{
    SpotLight()
        : PositionWS(0.0f, 0.0f, 0.0f, 1.0f)
        , PositionVS(0.0f, 0.0f, 0.0f, 1.0f)
        , DirectionWS(0.0f, 0.0f, 1.0f, 0.0f)
        , DirectionVS(0.0f, 0.0f, 1.0f, 0.0f)
        , Color(1.0f, 1.0f, 1.0f, 1.0f)
        , Range(1.0f)
        , SpotAngle(DirectX::XM_PIDIV2)
        , Intensity(1.0f)
        , LinearAttenuation(0.0f)
        , QuadraticAttenuation(0.0f)
    {}

    DirectX::XMFLOAT4 PositionWS;  // Light position in world space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 PositionVS;  // Light position in view space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 DirectionWS;  // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 DirectionVS;  // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Color;
    //----------------------------------- (16 byte boundary)
    float Range;
    float SpotAngle;
    float Intensity;
    float LinearAttenuation;
    //----------------------------------- (16 byte boundary)
    float QuadraticAttenuation;
    float Padding[3];
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 7 = 112 bytes
};

struct DirectionalLight
{
    DirectionalLight()
        : DirectionWS(0.0f, 0.0f, 1.0f, 0.0f)
        , DirectionVS(0.0f, 0.0f, 1.0f, 0.0f)
        , Color(1.0f, 1.0f, 1.0f, 1.0f)
        , Intensity(1.0f)
        , PositionWS(0.0f, 0.0f, 0.0f, 0.0f)
        , LightSpaceMatrix(Matrix4())
    {}
    Matrix4 LightSpaceMatrix;
    //----------------------------------- (64 byte boundary)
    DirectX::XMFLOAT4 DirectionWS;  // Light direction in world space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 DirectionVS;  // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 PositionWS;  // Light direction in view space.
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Color;
    //----------------------------------- (16 byte boundary)
    float Intensity;
    float Padding[3];
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 8 = 128 bytes
};

struct LightProperties
{
	uint32_t NumPointLights;
	uint32_t NumSpotLights;
	uint32_t NumDirectionalLights;
};