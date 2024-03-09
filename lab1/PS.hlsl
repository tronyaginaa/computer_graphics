Texture2D colorTexture : register(t0);
SamplerState colorSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 ps(VSOutput input) : SV_TARGET
{
    return float4(colorTexture.Sample(colorSampler, input.uv).xyz, 1.0);
}