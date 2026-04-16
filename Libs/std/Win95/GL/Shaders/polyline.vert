#version 330 core
#include <common.incl>

layout(location = 5) in vec4 inVertex1;
layout(location = 6) in vec4 inVertex2;
layout(location = 7) in vec4 inVertex3;
layout(location = 8) in vec4 inVertex4;
layout(location = 9) in vec4 inExtraLight;
layout(location = 10) in vec4 inUV01;
layout(location = 11) in vec4 inUV12;

out vec2 vTexCoord;
out vec4 vColor;


void main()
{
    vec4 vertices[4] = vec4[] (inVertex1, inVertex2, inVertex3, inVertex4);
    vec2 texCoords[4] = vec2[] (inUV01.xy, inUV01.zw, inUV12.xy, inUV12.zw);
    vec4 screenPos = vertices[gl_VertexID];
    vec3 viewPos = vertices[gl_VertexID].xyz;
    gl_Position = PROJECTION * vec4(viewPos, 1.0f);
    vTexCoord = texCoords[gl_VertexID];
    vec4 extraLight = inExtraLight;

    vColor.rgb = CalculateVertexColor(vec3(0), extraLight.rgb, iLightMode);
    vColor.a = extraLight.a;
}
