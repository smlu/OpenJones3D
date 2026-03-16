#version 330 core
#include <common.incl>

layout(location = 5) in mat4 inModelMatrix;
layout(location = 9) in vec4 inExtraLight;
layout(location = 10) in vec4 inSpriteOffset; //yVec for type 2
layout(location = 11) in vec4 inSpriteHalfSize;


uniform int spriteType;      // 0=camera, 2=custom axis

// Quad corners in local space (XY plane)
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

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec2 corner = quadCorners[gl_VertexID];
    vec2 sSize = inSpriteHalfSize.xy;
    vec3 sZVec = inSpriteOffset.xyz;
    vec4 sExtraLight = inExtraLight;


    if(spriteType == 0)
    {
        vec3 sOffset = inSpriteOffset.xyz;

        // local quad corner
        vec3 local = vec3(
        sOffset.x + corner.x * sSize.x,
        sOffset.y + corner.y * sSize.y,
        sOffset.z
        );

        gl_Position = PROJECTION * inModelMatrix * vec4(local, 1.0);

    }
    else if(spriteType == 2)
    {

        vec3 x;
        if (abs(sZVec.y) < 0.99989998)
        {
            x = normalize(cross(vec3(0.0, 1.0, 0.0), sZVec));
        }
        else
        {
            x = normalize(cross(vec3(1.0, 0.0, 0.0), sZVec));
        }

        vec3 y = normalize(cross(x, sZVec));

        vec3 offset = x * (corner.x * sSize.x) + y * (-corner.y * sSize.y);

        vec4 worldPos = inModelMatrix * vec4(offset, 1.0);
        gl_Position = VIEWPROJECTION * worldPos;
    }

    vTexCoord = quadUVs[gl_VertexID]; // UV mapping
    vColor.rgb = CalculateVertexColor(vec3(0), sExtraLight.rgb, iLightMode);
    vColor.a = sExtraLight.a;
}
