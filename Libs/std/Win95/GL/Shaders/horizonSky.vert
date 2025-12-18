#version 330 core
#include <globals.incl>

layout(location = 0) in vec4 inPosition; //given in CameraSpace
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSpecular;
layout(location = 3) in vec2 inTexCoord;

uniform vec3 camPYR;
uniform float horizonScale;


out vec2 vTexCoord;

vec3 clipToScreen(vec4 clip)
{
    // 1. Clip → NDC
    vec3 ndc = clip.xyz / clip.w;

    float width  = VIEWPORT.z - VIEWPORT.x;
    float height = VIEWPORT.w - VIEWPORT.y;

    vec3 screen;

    // 2. NDC → Screen X
    screen.x = ((ndc.x + 1.0) * 0.5) * width + VIEWPORT.x;

    // 3. NDC → Screen Y
    screen.y = (1.0 - (ndc.y + 1.0) * 0.5) * height + VIEWPORT.y;

    // 4. NDC → Screen Z
    screen.z = (ndc.z + 1.0) * 0.5;

    return screen;
}

void main()
{
    gl_Position = inPosition;

    vec2 screenSize = vec2(VIEWPORT.z - VIEWPORT.x, VIEWPORT.w - VIEWPORT.y);

    vec2 screenCenter = screenSize / 2.0f;

    vec2 screenPos = clipToScreen(inPosition).xy;

    float xFactor = (screenPos.x - screenCenter.x) * horizonScale;
    float yFactor = (screenPos.y - screenCenter.y) * horizonScale;


    float yaw = camPYR.y;
    float pitch = camPYR.x;

    float u = xFactor + yaw + inTexCoord.x;
    float v = yFactor + pitch + inTexCoord.y;

    vTexCoord = vec2(u, v);
}
