#version 450 core

// Types -----------------------------------------------------------------------

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_pos;
layout (location = 2) out vec4 out_norm;

// Constants -------------------------------------------------------------------

const bool k_distinguishActivePixels = true; // Makes certain "active" pixels brigher for visual clarity, but lowers performance
const float k_inactiveVal = k_distinguishActivePixels ? 1.0f / 3.0f : 1.0f;

// Uniforms --------------------------------------------------------------------

layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int u_swap;
    int u_geoCount;
    int u_airCount[2];
    ivec2 u_screenSize;
    vec2 u_screenAspectFactor;
    float u_sliceSize;
    float u_windSpeed;
    float u_dt;
    uint u_debug;
    ivec4 u_momentum;
    vec4 u_force;
    ivec4 u_dragForce;
    ivec4 u_dragMomentum;
};

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= u_screenAspectFactor; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= vec2(u_screenSize); // scale to texture space
    return screenPos;
}

void main() {
    out_color = vec4(k_inactiveVal, 0.0f, 0.0f, 0.0f);

    out_pos = vec4(in_pos, 0.0f);
    out_norm = vec4(normalize(in_norm), 0.0f);
            
    // Side View
    if (bool(u_debug)) {
        vec2 sideTexPos = worldToScreen(vec3(-in_pos.z - 1.0f, in_pos.y, 0));
        imageStore(u_sideImg, ivec2(sideTexPos), vec4(k_inactiveVal, 0.0f, 0.0f, 0.0f));
    }
}