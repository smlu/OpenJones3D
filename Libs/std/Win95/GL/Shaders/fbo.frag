#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D sSceneTexture;

void main() {
    FragColor = texture(sSceneTexture, texCoord);
}