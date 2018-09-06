#version 450 core

// Types -----------------------------------------------------------------------

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_norm;

// Constants -------------------------------------------------------------------

const bool k_distinguishActivePixels = true; // Makes certain "active" pixels brigher for visual clarity, but lowers performance
const float k_inactiveVal = k_distinguishActivePixels ? 1.0f / 3.0f : 1.0f;

// Uniforms --------------------------------------------------------------------

layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int u_swap;
    int u_geoCount;
    int u_airCount[2];
    int u_maxGeoPixels;
    int u_maxAirPixels;
    int u_screenSize;
    int u_slice;
    float u_sliceSize;
    float u_windSpeed;
    float u_dt;
    uint u_debug;
    vec4 u_lift;
    vec4 u_drag;
};

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec2 world) {
    return (world * 0.5f + 0.5f) * float(u_screenSize);
}

void main() {
    // Using the g and b channels to store world position
    vec2 subPixelPos = worldToScreen(in_pos.xy);
    subPixelPos -= gl_FragCoord.xy - 0.5f;
    out_color = vec4(k_inactiveVal, subPixelPos, 0.0f);

    out_norm = vec4(normalize(in_norm), 0.0f);
            
    // Side View
    if (bool(u_debug)) {
        vec2 sideTexPos = worldToScreen(vec2(-in_pos.z - 1.0f, in_pos.y));
        imageStore(u_sideImg, ivec2(sideTexPos), vec4(k_inactiveVal, 0.0f, 0.0f, 0.0f));
    }
}