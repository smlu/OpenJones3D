#version 330 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;      // actually BGRA
layout(location = 2) in vec4 inSpecular;   // optional
layout(location = 3) in vec2 inTexCoord;

uniform vec4 viewPort;

out vec4 vColor;
out vec2 vTexCoord;

vec4 screenToClip(vec3 screenPos, float rhw)
{
    float width  = viewPort.z - viewPort.x;
    float height = viewPort.w - viewPort.y;

    float w_clip = 1.0 / rhw;


    float ndc_x = ((screenPos.x - viewPort.x) / width)  * 2.0 - 1.0;
    float ndc_y = (1.0 - (screenPos.y - viewPort.y) / height) * 2.0 - 1.0;

    vec4 clip;
    clip.x = ndc_x * w_clip;
    clip.y = ndc_y * w_clip;
    clip.z = (screenPos.z * 2.0 - 1.0) * w_clip;
    clip.w = w_clip;

    return clip;
}

void main() {
    vColor = inColor.bgra; // DX->GL Farbreihenfolge
    vTexCoord = inTexCoord;

    vec4 clip = screenToClip(inPosition.xyz, inPosition.w);
    gl_Position = clip;
}
