#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec2 out_pos;

uniform vec2 u_origin;
uniform vec2 u_radius;

void main() {
    out_pos = in_pos;

    gl_Position = vec4(u_origin + u_radius * in_pos, 0.0f, 1.0f);
}