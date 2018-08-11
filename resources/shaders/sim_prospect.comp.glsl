#version 450 core

#define WORLD_POS_OFF 0
#define MOMENTUM_OFF 1
#define TEX_POS_OFF 2

#define MAX_GEO_PIXELS 32768

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Constants -------------------------------------------------------------------

const int k_estMaxAirPixels = 16384;
const int k_maxAirPixelsRows = 27 * 2;
const int k_maxAirPixelsSum = k_estMaxAirPixels * (k_maxAirPixelsRows / 2 / 3);

const int k_invocCount = 1024;

// Uniforms --------------------------------------------------------------------
                                
layout (binding = 0,   rgba8) uniform image2D u_fboImg;
layout (binding = 2, rgba32f) uniform image2D u_geoImg;
layout (binding = 5, rgba32f) uniform image2D u_fboPosImg;
layout (binding = 6, rgba32f) uniform image2D u_fboNormImg;

layout (binding = 0, offset = 0) uniform atomic_uint u_geoCount;

layout (binding = 0, std430) restrict buffer SSBO {
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
} ssbo;

struct GeoPixel {
    vec4 worldPos;
    vec4 normal;
};

layout (binding = 1, std430) buffer MapSSBO { // TODO: should be restrict?
    GeoPixel geoPixels[MAX_GEO_PIXELS];
    int map[k_maxAirPixelsSum];
} mapSSBO;

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

    mapSSBO.geoPixels[geoI].worldPos = geoWorldPos;
    mapSSBO.geoPixels[geoI].normal = geoNormal;  
}