#version 450 core

// Types -----------------------------------------------------------------------

// Constants -------------------------------------------------------------------

const bool k_distinguishActivePixels = true; // Makes certain "active" pixels brigher for visual clarity, but lowers performance
const float k_inactiveVal = k_distinguishActivePixels ? 1.0f / 3.0f : 1.0f;

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_norm;

// Uniforms --------------------------------------------------------------------

layout (binding = 4, rgba8) uniform image2D u_sideImg;

// Uniform buffer for better read-only performance
layout (binding = 0, std140) uniform Constants {
    int u_maxGeoPixels;
    int u_maxAirPixels;
    int u_screenSize;
    float u_liftC;
    float u_dragC;
    float u_windframeSize;
    float u_sliceSize;
    float u_windSpeed;
    float u_dt;
    int u_slice;
    float u_sliceZ;
    uint u_debug;
};

// Functions -------------------------------------------------------------------

vec2 windToScreen(vec2 wind) {
    return (wind / u_windframeSize + 0.5f) * float(u_screenSize);
}

void main() {
    // Using the g and b channels to store wind position
    vec2 subPixelPos = windToScreen(in_pos.xy);
    subPixelPos -= gl_FragCoord.xy - 0.5f;
    out_color = vec4(k_inactiveVal, subPixelPos, 0.0f);

    out_norm = vec4(normalize(in_norm), 0.0f);
            
    // Side View
    if (bool(u_debug)) {
        vec2 sideTexPos = windToScreen(vec2(-in_pos.z, in_pos.y));
        imageStore(u_sideImg, ivec2(sideTexPos), vec4(k_inactiveVal, 0.0f, 0.0f, 0.0f));
    }
}