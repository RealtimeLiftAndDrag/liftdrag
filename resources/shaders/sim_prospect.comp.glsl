#version 450 core

#define WORLD_POS_OFF 0
#define MOMENTUM_OFF 1
#define TEX_POS_OFF 2

#define DEBUG_SIZE 4096

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Constants -------------------------------------------------------------------

const int k_maxGeoPixels = 16384;
const int k_maxGeoPixelsRows = 27;
const int k_maxGeoPixelsSum = k_maxGeoPixels * (k_maxGeoPixelsRows / 3);

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

// Functions -------------------------------------------------------------------

ivec2 getGeoTexCoord(int index, int offset) { // with offset being 0, 1, or 2 (world, momentum, tex)
    return ivec2(
        index % k_maxGeoPixels,
        offset + 3 * (index / k_maxGeoPixels)
    );
}

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
    if (geoI >= k_maxGeoPixelsSum) {
        return;
    }

    vec4 geoWorldPos = imageLoad(u_fboPosImg, texCoord);
    vec4 geoNormal = imageLoad(u_fboNormImg, texCoord);

    imageStore(u_geoImg, getGeoTexCoord(geoI, WORLD_POS_OFF), geoWorldPos);
    imageStore(u_geoImg, getGeoTexCoord(geoI, MOMENTUM_OFF), geoNormal);      
}