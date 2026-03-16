#version 330 core
#include <common.incl>

layout(location = 9) in vec4 inExtraLight;
layout(location = 10) in vec4 inParticlePos; //w is particle half size

out vec2 vTexCoord;
out vec4 vColor;

const vec2 quadCorners[4] = vec2[](
vec2(-1.0, -1.0), // bottom-left
vec2( 1.0, -1.0), // bottom-right
vec2( 1.0,  1.0), // top-right
vec2(-1.0,  1.0)  // top-left
);

// Quad UVs
const vec2 quadUVs[4] = vec2[](
vec2( 0.0,  1.0), // bottom-left
vec2( 1.0,  1.0), // bottom-right
vec2( 1.0,  0.0), // top-right
vec2( 0.0,  0.0)  // top-left
);

void main()
{
    vec2 corner = quadCorners[gl_VertexID];
    float s = inParticlePos.w;

    vec3 view = inParticlePos.xyz;

    vec3 viewPos = view +
    vec3(
    corner.x * s,  // right
    corner.y * s,  // up
    0.0             // depth
    );

    gl_Position = PROJECTION * vec4(viewPos, 1.0);

    vTexCoord = quadUVs[gl_VertexID]; // UV mapping
    vColor.rgb = CalculateVertexColor(vec3(0), inExtraLight.rgb, iLightMode);
    vColor.a = inExtraLight.a;
}
