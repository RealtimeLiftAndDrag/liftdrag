#version 450 core

#define MAX_GEO_PIXELS 32768
#define MAX_AIR_PIXELS 32768

// Types -----------------------------------------------------------------------

struct GeoPixel {
    vec4 worldPos;
    vec4 normal;
};

struct AirPixel {
    vec4 worldPos;
    vec4 velocity;
};

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

layout (binding = 3, r32i) uniform coherent volatile iimage2D u_flagImg;
layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    coherent volatile int geoCount;
    coherent volatile int airCount[2];
    int _0; // padding
    ivec2 screenSize;
    vec2 screenAspectFactor;
    ivec4 momentum;
    ivec4 force;
    ivec4 dragForce;
    ivec4 dragMomentum;
} ssbo;

// Done this way because having a lot of large static sized arrays makes shader compilation super slow for some reason
layout (binding = 1, std430) buffer GeoPixels { // TODO: should be restrict?
    GeoPixel geoPixels[];
};
layout (binding = 2, std430) buffer AirPixels { // TODO: should be restrict?
    AirPixel airPixels[];
};
layout (binding = 3, std430) buffer AirGeoMap { // TODO: should be restrict?
    int airGeoMap[];
};

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= ssbo.screenAspectFactor; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= vec2(ssbo.screenSize); // scale to texture space
    return screenPos;
}

void main() {
    out_color = vec4(0.33f, 0.0f, 0.0f, 0.0f);

    out_pos = vec4(in_pos, 0.0f);
    out_norm = vec4(in_norm, 0.0f);
            
    // Side View
    vec2 sideTexPos = worldToScreen(vec3(-in_pos.z, in_pos.y, 0));
    imageStore(u_sideImg, ivec2(sideTexPos), vec4(0.33f, 0.0f, 0.0f, 0.0f));
}