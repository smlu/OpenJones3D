#version 330 core
#include <globals.incl>
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D sTexture;


void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    FragColor = texColor;
}
