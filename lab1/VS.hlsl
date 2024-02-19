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
    result.position = float4(vertex.position, 1.0f);
    result.color = vertex.color;

    return result;
}