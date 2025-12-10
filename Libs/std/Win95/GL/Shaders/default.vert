#version 330 core
#include <common.incl>

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;      // actually BGRA
layout(location = 2) in vec4 inSpecular;   // optional
layout(location = 3) in vec2 inTexCoord;


out vec4 vColor;
out vec2 vTexCoord;

void main() {
    vColor = inColor.bgra; // DX->GL Farbreihenfolge
    vTexCoord = inTexCoord;

    vec4 clip = screenToClip(inPosition.xyz, inPosition.w);
    gl_Position = clip;
}
