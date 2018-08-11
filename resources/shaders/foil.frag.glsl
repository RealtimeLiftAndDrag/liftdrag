#version 450 core

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_pos;
layout (location = 2) out vec4 out_norm;

// Constants -------------------------------------------------------------------

// Uniforms --------------------------------------------------------------------

layout (binding = 4, rgba8) uniform  image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO { 
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
} ssbo;

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= ssbo.screenSpec.zw; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= ssbo.screenSpec.xy; // scale to texture space
    return screenPos;
}

void main() {
    out_color = vec4(1.0f, 0.0f, 0.0f, 0.0f);
    out_pos = vec4(in_pos, 0.0f);
    out_norm = vec4(in_norm, 0.0f);
            
    // Side View
    vec2 sideTexPos = worldToScreen(vec3(-in_pos.z, in_pos.y, 0));
    imageStore(u_sideImg, ivec2(sideTexPos), vec4(1.0f, 0.0f, 0.0f, 0.0f));
}