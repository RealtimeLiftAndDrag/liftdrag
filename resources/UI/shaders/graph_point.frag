#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec4 out_color;

uniform vec3 u_color;

void main() {
    float r = in_pos.x * in_pos.x + in_pos.y * in_pos.y;
    out_color.rgb = vec3(u_color);
    out_color.a = max(1.0f - r * r, 0.0f);//step(r, 1.0f);
}