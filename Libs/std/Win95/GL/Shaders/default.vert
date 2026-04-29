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

uniform vec3 cExtraLight;
uniform float fAlpha;


void main() {

    vTexCoord = inTexCoord;

    vec4 clip = VIEWPROJECTION * inPosition;
    vWorldPos = inPosition.xyz;
    vWorldNormal = inNormal;
    vColor.rgb = CalculateVertexColor(inColor.rgb, cExtraLight, iLightMode);
    //vColor.rgb += CalculateLightColorLegacy(vWorldPos);
    vColor.a = inColor.a * fAlpha;


    gl_Position = clip;
}
