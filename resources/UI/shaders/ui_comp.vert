#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec2 out_pos;

uniform vec2 u_viewportSize;

void main() {
    out_pos = in_pos * u_viewportSize;

    gl_Position = vec4(in_pos * 2.0f - 1.0f, 0.0f, 1.0f);
}
