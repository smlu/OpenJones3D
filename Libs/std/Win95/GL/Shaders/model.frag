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

vec3 CalculateSectorLightColor()
{
    vec3 finalColor = vec3(0.0f);
    vec3 worldNormal = normalize(vWorldNormal);
    vec3 worldPos = vWorldPos;

    vec3 lightPos = vSecLightPos.xyz;
    vec3 lightDir = lightPos - worldPos;
    float maxRadius = vSecLightPos.a;
    float minRadius = vSecLightColor.a;

    float dist = length(lightDir);
    lightDir = normalize(lightDir);
    float dot = dot(worldNormal, lightDir);

    if (dist < maxRadius)
    {
        finalColor += vSecLightColor.rgb * dot;
    }
    else if (dist < minRadius)
    {
        float drad = minRadius - maxRadius;
        float atten = (dist - maxRadius) / drad;

        finalColor += (vSecLightColor.rgb - atten * vSecLightColor.rgb) * dot;

    }


    //float range = step(dist, light.maxRadius);
//    float fade = light.maxRadius * 0.5f;
//    float range = 1.0 - smoothstep(fade, light.maxRadius, dist);
//
//    float att = dist * 0.8f;
//    vec3 contrib = (light.color.rgb - vec3(att)) * range * dot;
//    contrib = max(contrib, vec3(0.0));
//    finalColor += contrib;

    return clamp(finalColor, vec3(0.0f), vec3(1.0f));

}

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = vColor;
    if (bRenderLights)
    {
        vertColor.xyz += CalculateModelLightColor(vWorldPos, normalize(vWorldNormal));
        vertColor.xyz += CalculateSectorLightColor();
    }
    vertColor = clamp(vertColor, vec4(0.0f), vec4(1.0f));
    vec4 col = vec4(vertColor * texColor);
    // Alpha-to-Coverage smoothing
    float alpha = col.a;

    if (bAlphaCut)
    {
        // Adaptive transition width
        //        float width = fwidth(alpha);
        //        width = max(width, 0.0001);
        //
        //        alpha = (alpha - 0.5f) / width + 0.5;
        //        //alpha = smoothstep(0.5f, 0.5f + width, col.a);
        //        alpha = clamp(alpha, 0.0, 1.0);
        if (alpha < 0.5f)
        {
            discard;
        }
    }
    else if (alpha < 0.01f)
    {
        discard;
    }

    // Alpha steuert MSAA Coverage
    FragColor = vec4(col.rgb, alpha);
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
    FragColor.rgb *= FADE_FACTOR;


}
