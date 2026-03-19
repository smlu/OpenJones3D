#version 330 core
#include <common.incl>

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vWorldPos;
in vec3 vWorldNormal;

uniform sampler2D sTexture;
uniform bool bRenderLights = false;
uniform bool bAlphaCut = false;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = vColor;
    if (bRenderLights)
    {
        //vertColor.xyz += CalculateModelLightColor(vWorldPos, normalize(vWorldNormal));
        vertColor.xyz += CalculateLightColor(vWorldPos);
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

    FragColor = vec4(col.rgb, alpha);
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
    FragColor.rgb *= FADE_FACTOR;

}