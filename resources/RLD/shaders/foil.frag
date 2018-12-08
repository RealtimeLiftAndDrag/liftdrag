#version 450 core

// Types -----------------------------------------------------------------------

// Constants -------------------------------------------------------------------

// External
const bool k_debug = DEBUG;
const bool k_distinguishActivePixels = DISTINGUISH_ACTIVE_PIXELS; // Makes certain "active" pixels brigher for visual clarity, but lowers performance
const bool k_doCloth = DO_CLOTH;

const uint k_geoBit = 1, k_airBit = 2, k_activeBit = 4; // Must also change in other shaders
const float k_inactiveVal = k_distinguishActivePixels && k_debug ? 1.0f / 3.0f : 1.0f;

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out uvec4 out_color;
layout (location = 1) out vec4 out_norm;
layout (location = 2) out uint out_index;

// Uniforms --------------------------------------------------------------------

layout (binding = 7, rgba8) uniform restrict image2D u_sideImg;

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

void main() {
    // Completely ignore any zero-normal geometry
    if (in_norm == vec3(0.0f)) {
        discard;
    }

    vec3 norm = normalize(in_norm);
    if (k_doCloth && !gl_FrontFacing) norm = -norm; // the normal is always facing forward, extremely necessary

    // Using the g and b channels to store wind position
    vec2 subPixelPos = windToScreen(in_pos.xy);
    subPixelPos -= gl_FragCoord.xy - 0.5f;
    out_color = uvec4(k_geoBit, uvec2(round(subPixelPos * 255.0f)), 0);
    out_norm = vec4(norm, 0.0f);
    if (k_doCloth) out_index = gl_PrimitiveID + 1; // TODO: if do tessellation, this will break

    // Side View
    if (k_debug) {
        //if (windToScreen(vec2(in_pos.x, 0.0f)).x > u_screenSize * 2 / 3) {
        vec2 sideTexPos = windToScreen(vec2(-in_pos.z, in_pos.y));
        imageStore(u_sideImg, ivec2(sideTexPos), vec4(vec3(k_inactiveVal * 0.5f), 0.0f));
        //}
    }
}