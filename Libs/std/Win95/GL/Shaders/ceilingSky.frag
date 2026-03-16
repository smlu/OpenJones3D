#version 330 core
#include <globals.incl>

out vec4 FragColor;

in vec3 vWorldPosition;

uniform sampler2D sTexture;
uniform float uCeilingZ;
uniform vec2 ceilingSkyOffset;

void main()
{
    vec3 normal = vec3(0.0f, -1.0f, 0.0f);
    vec3 cameraPositionWorld = INVERSE_VIEW[3].xyz;
    vec3 vert = vWorldPosition - cameraPositionWorld;
    vec3 skyVert = normalize(vert);
    vec3 ceiling = vec3(0.0f, uCeilingZ, 0.0f);

    vec3 diff = cameraPositionWorld - ceiling;

    float distanceToSphere = dot(diff, normal);
    float dott = -dot(skyVert, normal);

    float sphereHitDistance = distanceToSphere / dott;

    skyVert *= sphereHitDistance;
    skyVert += cameraPositionWorld;
    vec2 invTexSize = 1.0f / textureSize(sTexture, 0);

    float tu = invTexSize.x * skyVert.x * 16.0f + ceilingSkyOffset.x;
    float tv = invTexSize.y * -skyVert.z * 16.0f + ceilingSkyOffset.y;
    vec2 uv = vec2(tu, tv);
    vec4 texColor = texture(sTexture, uv);
    FragColor = texColor;
    FragColor.rgb *= FADE_FACTOR;
}
