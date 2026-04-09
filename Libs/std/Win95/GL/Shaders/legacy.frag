#version 330 core
#include <common.incl>

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sTexture;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = clamp(vColor, vec4(0), vec4(1));
    FragColor = vertColor * texColor;
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
}
