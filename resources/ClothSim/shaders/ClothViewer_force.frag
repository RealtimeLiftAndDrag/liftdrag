#version 450 core

// Inputs ----------------------------------------------------------------------

layout (location = 0) in float in_mag;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;

// Constants -------------------------------------------------------------------

// Functions -------------------------------------------------------------------

void main() {
    out_color.rgb = vec3(max(-in_mag, 0.0f), max(in_mag, 0.0f), 0.0f);
    out_color.a = 1.0f;
}
