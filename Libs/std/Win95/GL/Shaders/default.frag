#version 330 core
#include <common.incl>

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sTexture;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    if (texColor.a < 0.1f) {
        discard;
    }
    FragColor = vec4(vColor * texColor);
    if (FragColor.a <= 0.0f) {
        discard;
    }
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);

}