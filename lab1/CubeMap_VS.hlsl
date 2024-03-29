cbuffer SceneMatrixBuffer : register(b1)
{
    float4x4 viewProjectionMatrix;
    float4 cameraPos;
};

cbuffer WorldMatrixBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4 size;
};

struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 localPos : POSITION1;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float3 pos = cameraPos.xyz + input.position * size.x;
    output.position = mul(viewProjectionMatrix, mul(worldMatrix, float4(pos, 1.0f)));
    output.localPos = input.position;

    return output;
}