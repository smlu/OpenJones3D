#version 330 compatibility

out vec2 texCoord;

uniform bool bMirrorY = false;

void main()
{
    vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;
    texCoord = (position + 1) * 0.5;
    if (bMirrorY)
    {
        texCoord.y = 1 - texCoord.y;
    }
    gl_Position = vec4(position, 0, 1);
}
