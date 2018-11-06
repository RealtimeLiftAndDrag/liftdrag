#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec4 out_color;

uniform vec2 u_viewportSize;
uniform vec4 u_backColor;
uniform vec4 u_borderColor;

float isWithin(vec2 p, vec2 bottomLeft, vec2 topRight) {
    vec2 s = step(bottomLeft, p) - step(topRight, p);
    return s.x * s.y;
}

void main() {
    float border = 1.0f - isWithin(in_pos, vec2(1.0f), vec2(u_viewportSize - 1.0f));

    out_color = mix(u_backColor, u_borderColor, border);
}