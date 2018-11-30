#version 450 core

layout (location = 0) in float in_stress;

layout (location = 0) out vec4 out_color;

const vec3 k_neutralColor = vec3(0.5f);
const vec3 k_stretchColor = vec3(1.0f, 0.0f, 0.0f);
const vec3 k_compressColor = vec3(0.0f, 0.5f, 1.0f);
const float k_multiplier = 3.0f;

void main() {
    out_color.rgb = in_stress > 0.0f ?
        mix(k_neutralColor, k_stretchColor, in_stress * k_multiplier) : 
        mix(k_neutralColor, k_compressColor, -in_stress * k_multiplier);
    out_color.a = 1.0f;
}
