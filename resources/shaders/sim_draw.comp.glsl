#version 450 core

#define WORLD_POS_OFF 0
#define MOMENTUM_OFF 1
#define TEX_POS_OFF 2

#define DEBUG_SIZE 4096

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// Constants -------------------------------------------------------------------

const int k_maxGeoPixels = 16384;
const int k_maxGeoPixelsRows = 27;
const int k_maxGeoPixelsSum = k_maxGeoPixels * (k_maxGeoPixelsRows / 3);

const int k_estMaxAirPixels = 16384;
const int k_maxAirPixelsRows = 27 * 2;
const int k_maxAirPixelsSum = k_estMaxAirPixels * (k_maxAirPixelsRows / 2 / 3);

const int k_invocCount = 1024;

const vec3 k_sideGeoColor = vec3(1.0f, 0.0f, 1.0f);
const vec3 k_sideAirColor = vec3(0.0f, 1.0f, 0.0f);

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0,   rgba8) uniform  image2D u_fboImg;
layout (binding = 1,    r32i) uniform iimage2D u_flagImg;
layout (binding = 2, rgba32f) uniform  image2D u_geoImg;
layout (binding = 3, rgba32f) uniform  image2D u_airImg;
layout (binding = 4,   rgba8) uniform  image2D u_geoSideImg;

layout (binding = 0, offset = 0) uniform atomic_uint u_geoCount;
layout (binding = 0, offset = 4) uniform atomic_uint u_airCount[2];

layout (binding = 0, std430) restrict buffer SSBO {
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
} ssbo;

layout (binding = 1, std430) buffer MapSSBO { // TODO: should be restrict?
    int map[k_maxAirPixelsSum];
} mapSSBO;

// Functions -------------------------------------------------------------------

ivec2 getGeoTexCoord(int index, int offset) { // with offset being 0, 1, or 2 (world, momentum, tex)
    return ivec2(
        index % k_maxGeoPixels,
        offset + 3 * (index / k_maxGeoPixels)
    );
}

ivec2 getAirTexCoord(int index, int offset, int swap) { // with offset being 0, 1, or 2  (world, momentum, tex)
    return ivec2(
        index % k_estMaxAirPixels,
        offset + 3 * (index / k_estMaxAirPixels) + swap * (k_maxAirPixelsRows / 2)
    );
}

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= ssbo.screenSpec.zw; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= ssbo.screenSpec.xy; // scale to texture space
    return screenPos;
}

void main() {
    int airCount = int(atomicCounter(u_airCount[u_swap]));
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (airCount + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {

        int airI = invocI + (k_invocCount * ii);
        if (airI >= airCount) {
            break;
        }
        int geoI = mapSSBO.map[airI];

        vec3 geoWorldPos = imageLoad(u_geoImg, getGeoTexCoord(geoI, WORLD_POS_OFF)).xyz; // TODO: this needs to not be airI
        vec3 geoNormal = imageLoad(u_geoImg, getGeoTexCoord(geoI, MOMENTUM_OFF)).xyz;
        vec3 airWorldPos = imageLoad(u_airImg, getAirTexCoord(airI, WORLD_POS_OFF, u_swap)).xyz;
        vec3 airVelocity = imageLoad(u_airImg, getAirTexCoord(airI, MOMENTUM_OFF, u_swap)).xyz;
        
        ivec2 airScreenPos = ivec2(worldToScreen(airWorldPos));
        ivec2 geoSideTexPos = ivec2(worldToScreen(vec3(-geoWorldPos.z, geoWorldPos.y, 0)));
        ivec2 airSideTexPos = ivec2(worldToScreen(vec3(-airWorldPos.z, airWorldPos.y, 0)));
        
        vec4 color = imageLoad(u_fboImg, airScreenPos);
        color.g = 1.0f;

        imageStore(u_fboImg, airScreenPos, color);        
        imageStore(u_geoSideImg, geoSideTexPos, vec4(k_sideGeoColor, 1.0f));
        imageStore(u_geoSideImg, airSideTexPos, vec4(k_sideAirColor, 1.0f));
        imageAtomicExchange(u_flagImg, airScreenPos, airI + 1);
    }        
}