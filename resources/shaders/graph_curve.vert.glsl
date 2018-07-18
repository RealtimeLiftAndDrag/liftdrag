#version 450 core

layout (location = 0) in vec2 in_point;

uniform vec2 u_min;
uniform vec2 u_max;

void main() {
    gl_Position = vec4((in_point - u_min) / (u_max - u_min) * 2.0f - 1.0f, 0.0f, 1.0f);
}