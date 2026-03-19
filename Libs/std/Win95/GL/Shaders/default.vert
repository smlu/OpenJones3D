#version 330 core
#include <common.incl>

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;      // actually BGRA
layout(location = 2) in vec4 inSpecular;   // optional
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inNormal;


out vec4 vColor;
out vec2 vTexCoord;
out vec3 vWorldPos;
out vec3 vWorldNormal;

uniform bool bRenderLights = false;
uniform vec3 cExtraLight;
uniform float fAlpha;


void main() {

    vTexCoord = inTexCoord;

    vec4 clip = vec4(0.0f);
    if (iVertexSpace == VS_SCREEN)
    {
        clip = screenToClip(inPosition.xyz, inPosition.w);
//        clip.z = -clip.w;
        vColor = inColor;
    }
    else if (iVertexSpace == VS_WORLD)
    {
        clip = VIEWPROJECTION * inPosition;
        vWorldPos = inPosition.xyz;
        vWorldNormal = inNormal;
        vColor.rgb = CalculateVertexColor(inColor.rgb, cExtraLight, iLightMode);
        vColor.a = inColor.a * fAlpha;
//        if (bRenderLights)
//        {
//            vColor.rgb += CalculateLightColor(vWorldPos);
//        }
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
