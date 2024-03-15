struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return float4(input.color.xyz, 0.5f);
}