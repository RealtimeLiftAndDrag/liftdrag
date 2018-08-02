#version 450 core

layout (location = 0) in vec2 in_pos;

out vec2 v2f_pos;

uniform vec2 u_viewMin;
uniform vec2 u_viewMax;
uniform vec2 u_gridSize;
uniform vec2 u_viewportSize;

void main() {
    v2f_pos = in_pos * 0.5f + 0.5f;

    gl_Position = vec4(in_pos, 0.0f, 1.0f);
}