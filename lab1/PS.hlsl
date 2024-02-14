struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 ps(VSOutput input) : SV_TARGET
{
    return input.color;
}