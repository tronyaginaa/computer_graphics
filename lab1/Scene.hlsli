#include "Constants.h"

struct LIGHT
{
    float4 lightPos;
    float4 lightColor;
};

cbuffer LightBuffer : register(b2)
{
    float4 cameraPos;
    int4 lightParams;
    LIGHT lights[MAX_LIGHTS];
    float4 ambientColor;
};