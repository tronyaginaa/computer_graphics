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
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;
    result.position = mul(viewProjectionMatrix, mul(worldMatrix, float4(vertex.position, 1.0f)));
    result.color = vertex.color;

    return result;
}