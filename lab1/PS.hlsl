#include "Cube.hlsli"

Texture2DArray cubeTexture : register(t0);
Texture2D cubeNormal : register(t1);
SamplerState cubeSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 worldPos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    nointerpolation uint instanceId : INST_ID;
};

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 color = cubeTexture.Sample(cubeSampler, float3(input.uv, geomBuffer[input.instanceId].shineSpeedTexIdNM.y)).xyz;
    float3 finalColor = ambientColor.xyz * color;

    float3 norm = float3(0, 0, 0);
    if (lightParams.y > 0 && geomBuffer[input.instanceId].shineSpeedTexIdNM.z > 0.0f)
    {
        float3 binorm = normalize(cross(input.normal, input.tangent));
        float3 localNorm = cubeNormal.Sample(cubeSampler, input.uv).xyz * 2.0 - 1.0;
        norm = localNorm.x * normalize(input.tangent) + localNorm.y * binorm + localNorm.z * normalize(input.normal);
    }
    else
    {
        norm = input.normal;
    }

    return float4(CalculateColor(finalColor, norm, input.worldPos.xyz, geomBuffer[input.instanceId].shineSpeedTexIdNM.x, false), 1.0);
}