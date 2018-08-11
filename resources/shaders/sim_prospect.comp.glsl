#version 450 core

#define MAX_GEO_PIXELS 32768
#define MAX_AIR_PIXELS 32768

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Types -----------------------------------------------------------------------

struct GeoPixel {
    vec4 worldPos;
    vec4 normal;
};

struct AirPixel {
    vec4 worldPos;
    vec4 velocity;
};

// Constants -------------------------------------------------------------------

const int k_invocCount = 1024;

// Uniforms --------------------------------------------------------------------
                                
layout (binding = 0,   rgba8) uniform image2D u_fboImg;
layout (binding = 5, rgba32f) uniform image2D u_fboPosImg;
layout (binding = 6, rgba32f) uniform image2D u_fboNormImg;

layout (binding = 0, offset = 0) uniform atomic_uint u_geoCount;

layout (binding = 0, std430) restrict buffer SSBO {
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
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

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    if (texCoord.x >= int(ssbo.screenSpec.x) || texCoord.y >= int(ssbo.screenSpec.y)) {
        return;
    }   

    vec4 col = imageLoad(u_fboImg, texCoord);
    if (col.r == 0.0f) {
        return;
    }

    int geoI = int(atomicCounterIncrement(u_geoCount));
    if (geoI >= MAX_GEO_PIXELS) {
        return;
    }

    vec4 geoWorldPos = imageLoad(u_fboPosImg, texCoord);
    vec4 geoNormal = imageLoad(u_fboNormImg, texCoord);

    geoPixels[geoI].worldPos = geoWorldPos;
    geoPixels[geoI].normal = geoNormal;  
}