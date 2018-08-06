#version 450 core

layout (location = 0) in vec2 in_point;

out vec2 v2f_point;

uniform vec2 u_viewMin;
uniform vec2 u_viewMax;

void main() {
    v2f_point = in_point;

    gl_Position = vec4((in_point - u_viewMin) / (u_viewMax - u_viewMin) * 2.0f - 1.0f, 0.0f, 1.0f);
}
