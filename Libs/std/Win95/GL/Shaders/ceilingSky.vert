#version 330 core
#include <globals.incl>

layout(location = 0) in vec4 inPosition; //given in CameraSpace
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSpecular;
layout(location = 3) in vec2 inTexCoord;

out vec3 vWorldPosition;
out vec2 vTexCoord;

void main() {
    vWorldPosition = inPosition.xyz;
    vTexCoord = inTexCoord;
    vec4 clip = PROJECTION * VIEW * inPosition;
    clip.z = clip.w;
    gl_Position = clip;
}
