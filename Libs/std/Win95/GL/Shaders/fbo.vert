#version 330 core

out vec2 texCoord;

void main()
{
    vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;
    texCoord = (position + 1) * 0.5;
    gl_Position = vec4(position, 0, 1);
}
