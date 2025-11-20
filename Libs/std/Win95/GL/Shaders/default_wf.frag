#version 330 compatibility

in vec4 vColor;

out vec4 FragColor;

uniform vec4 vWireFrameColor;


void main()
{
    FragColor = vColor * vWireFrameColor;
}
