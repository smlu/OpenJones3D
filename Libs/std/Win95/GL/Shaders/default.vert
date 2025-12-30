#version 330 core
#include <common.incl>

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;      // actually BGRA
layout(location = 2) in vec4 inSpecular;   // optional
layout(location = 3) in vec2 inTexCoord;


out vec4 vColor;
out vec2 vTexCoord;

void main() {
    vColor = inColor;
    vTexCoord = inTexCoord;

    vec4 clip = vec4(0.0f);
    if (iVertexSpace == VS_SCREEN)
    {
        clip = screenToClip(inPosition.xyz, inPosition.w);
        //clip.z = -clip.w;
    }
    else if (iVertexSpace == VS_WORLD)
    {
        clip = VIEWPROJECTION * inPosition;
    }
    else if (iVertexSpace == VS_VIEW)
    {
        clip = PROJECTION * inPosition;
    }
    else if (iVertexSpace == VS_CLIP)
    {
        clip = inPosition;
    }

    gl_Position = clip;
}
