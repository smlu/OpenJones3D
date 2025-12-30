#version 330 core
#include <globals.incl>

layout(location = 0) in vec4 inPosition; //given in WorlsSpace
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSpecular;
layout(location = 3) in vec2 inTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord = inTexCoord;

    vec4 clip = VIEWPROJECTION * vec4(inPosition.xyz, 1.0f);
    clip.z = clip.w;
    gl_Position = clip;
}