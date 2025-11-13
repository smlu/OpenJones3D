#version 330 compatibility

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sTexture;

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    if (texColor.a < 0.1f) {
        discard;
    }
    FragColor = vec4(vColor * texColor);

}