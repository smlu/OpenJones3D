#version 330 core
#include <globals.incl>

layout(location = 0) in vec4 inPosition; //given in CameraSpace
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSpecular;

out vec3 vWorldPosition;

void main() {
    vWorldPosition = inPosition.xyz;
    vec4 clip = VIEWPROJECTION * inPosition;
    clip.z = clip.w;
    gl_Position = clip;
}
