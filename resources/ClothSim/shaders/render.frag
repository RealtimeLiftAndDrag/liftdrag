#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

layout (location = 0) out vec4 out_color;

uniform vec3 u_lightDir; // normalized vector towards light
uniform vec3 u_camPos; // camera position in world space

const float k_ambience = 0.2f;

void main() {
    vec3 norm = normalize(in_norm);

    float diffuse = dot(u_lightDir, norm) * (1.0f - k_ambience) + k_ambience;

    out_color.rgb = vec3(diffuse);
    out_color.a = 1.0f;
}
