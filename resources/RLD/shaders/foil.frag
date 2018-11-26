#version 450 core

#ifndef DEBUG
#define DEBUG false
#endif

#ifndef DISTINGUISH_ACTIVE_PIXELS
#define DISTINGUISH_ACTIVE_PIXELS false
#endif

// Types -----------------------------------------------------------------------

// Constants -------------------------------------------------------------------

const uint k_geoBit = 1, k_airBit = 2, k_activeBit = 4; // Must also change in other shaders
const bool k_debug = DEBUG;
const bool k_distinguishActivePixels = DISTINGUISH_ACTIVE_PIXELS; // Makes certain "active" pixels brigher for visual clarity, but lowers performance
const float k_inactiveVal = k_distinguishActivePixels && k_debug ? 1.0f / 3.0f : 1.0f;

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out uvec4 out_color;
layout (location = 1) out vec4 out_norm;

// Uniforms --------------------------------------------------------------------

layout (binding = 7, rgba8) uniform image2D u_sideImg;

// Uniform buffer for better read-only performance
layout (binding = 0, std140) uniform Constants {
    int u_maxGeoPixels;
    int u_maxAirPixels;
    int u_screenSize;
    float u_liftC;
    float u_dragC;
    float u_windframeSize;
    float u_windframeDepth;
    float u_sliceSize;
    float u_turbulenceDist;
    float u_maxSearchDist;
    float u_windShadDist;
    float u_backforceC;
    float u_flowback;
    float u_initVelC;
    float u_windSpeed;
    float u_dt;
    int u_slice;
    float u_sliceZ;
    float u_pixelSize;
};

// Functions -------------------------------------------------------------------

vec2 windToScreen(vec2 wind) {
    return (wind / u_windframeSize + 0.5f) * float(u_screenSize);
}

vec3 safeNormalize(vec3 v) {
    float d = dot(v, v);
    return d > 0.0f ? v / sqrt(d) : vec3(0.0f);
}

void main() {
    // Completely ignore any zero-normal geometry
    if (in_norm == vec3(0.0f)) {
        discard;
    }
    // Using the g and b channels to store wind position
    vec2 subPixelPos = windToScreen(in_pos.xy);
    subPixelPos -= gl_FragCoord.xy - 0.5f;
    out_color = uvec4(k_geoBit, uvec2(round(subPixelPos * 255.0f)), 0);
    out_norm = vec4(normalize(in_norm), 0.0f);

    // Side View
    if (k_debug) {
        //if (windToScreen(vec2(in_pos.x, 0.0f)).x > u_screenSize * 2 / 3) {
        vec2 sideTexPos = windToScreen(vec2(-in_pos.z, in_pos.y));
        imageStore(u_sideImg, ivec2(sideTexPos), vec4(vec3(k_inactiveVal * 0.5f), 0.0f));
        //}
    }
}