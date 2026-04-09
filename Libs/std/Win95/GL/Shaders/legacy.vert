#version 330 core
#include <common.incl>

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSpecular;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inNormal;


out vec4 vColor;
out vec2 vTexCoord;

uniform vec3 cExtraLight = vec3(0.0);


void main() {

    vTexCoord = inTexCoord;
    vec4 clip = screenToClip(inPosition.xyz, inPosition.w);
    vColor = inColor;
    vColor.rgb += cExtraLight;

    gl_Position = clip;
}
