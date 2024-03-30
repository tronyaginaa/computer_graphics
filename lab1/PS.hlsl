#include "ColorCalc.hlsli"

Texture2D colorTexture : register(t0);
Texture2D normals : register(t1);
SamplerState colorSampler : register(s0);

cbuffer WorldMatrixBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4 shine;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 worldPos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 color = colorTexture.Sample(colorSampler, input.uv).xyz;
    float3 finalColor = ambientColor.xyz * color;

    float3 norm = float3(0, 0, 0);
    if (lightParams.y > 0)
    {
        float3 binorm = normalize(cross(input.normal, input.tangent));
        float3 localNorm = normals.Sample(colorSampler, input.uv).xyz * 2.0 - 1.0;
        norm = localNorm.x * normalize(input.tangent) + localNorm.y * binorm + localNorm.z * normalize(input.normal);
    }
    else
    {
        norm = input.normal;
    }

    return float4(CalculateColor(color, norm, input.worldPos.xyz, shine.x, false), 1.0);
}