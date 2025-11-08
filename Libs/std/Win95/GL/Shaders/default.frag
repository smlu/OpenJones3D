#version 330 compatibility

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in vec4 vPosition;
flat in vec2 viewPortSize;

uniform sampler2D sTexture;

uniform vec4 viewPort;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    if (viewPort.z == 0.0f) {
        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        FragColor = vec4(vColor * texColor);
    }

}