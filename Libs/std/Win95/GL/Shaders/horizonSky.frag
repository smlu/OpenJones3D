#version 330 core
#include <globals.incl>
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D sTexture;
uniform vec3 camPYR;
uniform float horizonScale;


void main()
{
    vec2 screenSize = vec2(VIEWPORT.z - VIEWPORT.x, VIEWPORT.w - VIEWPORT.y);

    vec2 screenPosition = (gl_FragCoord.xy - VIEWPORT.xy) / screenSize;

    vec2 screenCenter = screenSize / 2.0f;

    float xFactor = (gl_FragCoord.x - screenCenter.x) * horizonScale;
    float yFactor = (screenCenter.y - gl_FragCoord.y) * horizonScale;


    float yaw = camPYR.y;
    float pitch = camPYR.x;

    float u = xFactor + yaw + vTexCoord.x;
    float v = yFactor + pitch + vTexCoord.y;

    vec4 texColor = texture(sTexture, vec2(u, v));
    FragColor = texColor;
}
