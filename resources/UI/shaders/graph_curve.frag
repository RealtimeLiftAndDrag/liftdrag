#version 450 core

layout (location = 0) out vec4 out_color;

uniform vec3 u_color;

void main() {
    out_color.rgb = u_color;
    out_color.a = 1.0f;
}