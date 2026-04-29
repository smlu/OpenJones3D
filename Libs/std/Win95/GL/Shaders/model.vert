#version 330 core
#include <common.incl>

#define MAX_BATCHES 32

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;      // actually BGRA
layout(location = 2) in vec4 inSpecular;   // optional
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inNormal;
layout(location = 5) in mat4 inModelMatrix;
layout(location = 9) in vec4 inExtraLight;
layout(location = 10) in vec4 inSecLightPos;
layout(location = 11) in vec4 inSecLightColor;


out vec4 vColor;
out vec2 vTexCoord;
out vec3 vWorldPos;
out vec3 vWorldNormal;
flat out vec4 vSecLightPos;
flat out vec4 vSecLightColor;

uniform bool bRenderLights = false;


void main() {
    vColor.rgb = CalculateVertexColor(inColor.rgb, inExtraLight.rgb, iLightMode);
    vColor.a = inColor.a * inExtraLight.a;
    vTexCoord = inTexCoord;
    vec4 normal = inModelMatrix * vec4(inNormal, 0.0f);
    vWorldNormal = normal.xyz;


    vec4 worldPos = inModelMatrix * inPosition;
    vWorldPos = worldPos.xyz;

    vSecLightPos = inSecLightPos;
    vSecLightColor = inSecLightColor;

//    vColor.rgb += CalculateModelLightingLegacy(vWorldPos, vWorldNormal);
//    PointLight light;
//    light.maxRadius = vSecLightPos.a;
//    light.minRadius = vSecLightColor.a;
//    light.position = vSecLightPos;
//    light.color = vSecLightColor;
//    vColor.rgb += CalculateModelLightColorLegacy(light, vWorldPos, vWorldNormal);

    gl_Position = VIEWPROJECTION * worldPos;
}
