#version 330 core
#include <common.incl>

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sTexture;
uniform bool bAlphaCut = false;


void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    vec4 vertColor = clamp(vColor, vec4(0), vec4(1));
    FragColor = vertColor * texColor;
    if ((bAlphaCut && FragColor.a < 0.5f) || FragColor.a < 0.01f)
    {
        discard;
    }
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);
}
