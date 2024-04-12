#include "Scene.hlsli"

float3 CalculateColor(in float3 objColor, in float3 objNormal, in float3 pos, in float shine, in bool transparent)
{
    float3 finalColor = float3(0, 0, 0);

    if (lightParams.z > 0)
    {
        return float3(objNormal * 0.5 + float3(0.5, 0.5, 0.5));
    }

    for (int i = 0; i < lightParams.x; i++)
    {
        float3 norm = objNormal;

        float3 lightDir = lights[i].lightPos.xyz - pos;
        float lightDist = length(lightDir);
        lightDir /= lightDist;

        float atten = clamp(1.0 / (lightDist * lightDist), 0, 1);

        if (transparent && dot(lightDir, objNormal) < 0.0)
        {
            norm = -norm;
        }
        finalColor += objColor * max(dot(lightDir, norm), 0) * atten * lights[i].lightColor.xyz;

        float3 viewDir = normalize(cameraPos.xyz - pos);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = shine > 0 ? pow(max(dot(viewDir, reflectDir), 0.0), shine.x) : 0.0;

        finalColor += objColor * spec * lights[i].lightColor.xyz;
    }

    return finalColor;
}