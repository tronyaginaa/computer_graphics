cbuffer WorldMatrixBuffer : register(b0)
{
    float4x4 worldMatrix;
};

cbuffer SceneMatrixBuffer : register(b1)
{
    float4x4 viewProjectionMatrix;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;
    result.position = mul(viewProjectionMatrix, mul(worldMatrix, float4(vertex.position, 1.0f)));
    result.uv = vertex.uv;

    return result;
}