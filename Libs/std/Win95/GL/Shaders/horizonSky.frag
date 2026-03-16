#version 330 core
#include <globals.incl>
out vec4 FragColor;

uniform sampler2D sTexture;
uniform vec3 camPYR;
uniform float horizonScale;
uniform vec2 horizonSkyOffset;
uniform vec2 uvOffset;


void main()
{
    vec2 screenSize = vec2(VIEWPORT.z - VIEWPORT.x, VIEWPORT.w - VIEWPORT.y);

    vec2 screenPosition = (gl_FragCoord.xy - VIEWPORT.xy) / screenSize;

    vec2 screenCenter = screenSize / 2.0f;

    float xFactor = (gl_FragCoord.x - screenCenter.x) * horizonScale;
    float yFactor = (screenCenter.y - gl_FragCoord.y) * horizonScale;


    float yaw = camPYR.y;
    float pitch = camPYR.x;

    float u = xFactor + yaw + horizonSkyOffset.x + uvOffset.x;
    float v = yFactor + pitch + horizonSkyOffset.y + uvOffset.y;

    vec4 texColor = texture(sTexture, vec2(u, v));
    FragColor = texColor;
    FragColor.rgb *= FADE_FACTOR;
}
