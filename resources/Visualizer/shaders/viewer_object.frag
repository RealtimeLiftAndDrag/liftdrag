#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 norm = normalize(in_norm);

    out_color.rgb = (norm + 1.0f) * 0.5f;
    out_color.a = 1.0f;
}
