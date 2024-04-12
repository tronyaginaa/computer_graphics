#include "constants.h"
#include "ColorCalc.hlsli"


struct GeomBuffer
{
    float4x4 worldMatrix;
    float4x4 norm;
    float4 shineSpeedTexIdNM;
};

cbuffer GeomBufferInst : register(b0)
{
    GeomBuffer geomBuffer[MAX_CUBES];
};

cbuffer SceneConstantBuffer : register(b1)
{
    float4x4 viewProjectionMatrix;
    int4 indexBuffer[MAX_CUBES];
};