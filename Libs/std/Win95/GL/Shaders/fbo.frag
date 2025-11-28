#version 330 compatibility

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D sSceneTexture;
uniform vec2 camPlanes;

float LinearDepth(float depthValue) {
    float z_ndc = depthValue * 2.0 - 1.0; // map back to NDC z
    // viewZ (eye space z) using standard projection:
    float viewZ = (2.0 * camPlanes.x * camPlanes.y) / (camPlanes.y + camPlanes.x - z_ndc * (camPlanes.y - camPlanes.x));
    // normalize
    return (-viewZ) / camPlanes.y;
}

void main() {
    FragColor = texture(sSceneTexture, texCoord);
    FragColor.r = LinearDepth(texture(sSceneTexture, texCoord));
}