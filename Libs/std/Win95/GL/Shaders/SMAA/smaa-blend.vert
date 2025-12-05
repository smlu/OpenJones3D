#version 330 core
#define mad(a, b, c) (a * b + c)

uniform vec2 uTexelSize;

out vec2 vTexCoord0;
out vec4 vOffset;

void main() {

    vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;
    vTexCoord0 = (position + 1) * 0.5;

	vOffset = mad(vec4(uTexelSize, uTexelSize), vec4(1.0, 0.0, 0.0,  1.0), vTexCoord0.xyxy);

    gl_Position = vec4(position, 0.0, 1.0);
}
