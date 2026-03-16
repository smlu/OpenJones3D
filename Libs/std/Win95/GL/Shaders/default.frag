#version 330 core
#include <common.incl>

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vWorldPos;

uniform sampler2D sTexture;
uniform bool bRenderLights = false;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = vColor;
    if (bRenderLights)
    {
        vertColor.xyz += CalculateLightColor(vWorldPos);
    }
    vertColor = clamp(vertColor, vec4(0.0f), vec4(1.0f));
    FragColor = vec4(vertColor * texColor);
    if (FragColor.a <= 0.0f) {
        discard;
    }
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
    FragColor.rgb *= FADE_FACTOR;

}