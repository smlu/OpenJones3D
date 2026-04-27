#version 330 core
#include <common.incl>

#define MAX_BATCHES 32

layout(std140) uniform SectorLightsData
{
    PointLight SECTOR_LIGHTS[MAX_BATCHES];
};

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vWorldPos;
in vec3 vWorldNormal;
flat in vec4 vSecLightPos;
flat in vec4 vSecLightColor;

uniform sampler2D sTexture;
uniform bool bRenderLights = false;
uniform bool bAlphaCut = false;
uniform bool bAlphaToCoverage = false;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = vColor;
    if (bRenderLights)
    {
        vertColor.rgb += CalculateModelLighting(vWorldPos, vWorldNormal);
        PointLight light;
        light.maxRadius = vSecLightPos.a;
        light.minRadius = vSecLightColor.a;
        light.position = vSecLightPos;
        light.color = vSecLightColor;
        vertColor.rgb += CalculateModelLightColor(light, vWorldPos, vWorldNormal);
    }
    vertColor = clamp(vertColor, vec4(0.0f), vec4(1.0f));
    vec4 col = vec4(vertColor * texColor);
    // Alpha-to-Coverage smoothing
    float alpha = col.a;

    if (bAlphaCut && bAlphaToCoverage)
    {
        // Adaptive transition width
        float width = fwidth(alpha);
        width = max(width, 0.0001);

        alpha = (alpha - 0.5f) / width + 0.5;
        alpha = clamp(alpha, 0.0, 1.0);
    }
    else if ((bAlphaCut && alpha < 0.5f) || alpha < 0.01f)
    {
        discard;
    }

    FragColor = vec4(col.rgb, alpha);
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
    FragColor.rgb *= FADE_FACTOR;


}
