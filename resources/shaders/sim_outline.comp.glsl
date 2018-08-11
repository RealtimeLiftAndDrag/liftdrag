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

const int k_maxSteps = 50;

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0,   rgba8) uniform  image2D u_fboImg;
layout (binding = 1,    r32i) uniform iimage2D u_flagImg;
layout (binding = 2, rgba32f) uniform  image2D u_geoImg;
layout (binding = 3, rgba32f) uniform  image2D u_airImg;

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

vec2 screenToWorldDir(vec2 screenDir) {
    return screenDir / (min(ssbo.screenSpec.x, ssbo.screenSpec.y) * 0.5f);
}

void main() {
    int geoCount = int(atomicCounter(u_geoCount));
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (geoCount + k_invocCount - 1) / k_invocCount;    
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int geoI = invocI + (k_invocCount * ii);
        if (geoI >= geoCount || geoI >= k_maxGeoPixelsSum) {
            break;
        }

        vec3 geoWorldPos = imageLoad(u_geoImg, getGeoTexCoord(geoI, WORLD_POS_OFF)).xyz;
        vec3 geoNormal = imageLoad(u_geoImg, getGeoTexCoord(geoI, MOMENTUM_OFF)).xyz;

        vec2 screenPos = floor(worldToScreen(geoWorldPos)) + 0.5f;

        // Check if actually a geometry pixel
        // TODO: this should not be necessary
        vec4 col = imageLoad(u_fboImg, ivec2(screenPos));
        if (col.r == 0.0f) {
            continue;
        }

        vec2 screenDir = normalize(geoNormal.xy);

        // TODO: magic numbers
        bool shouldSpawn = geoNormal.z >= 0.01f && geoNormal.z <= 0.99f && geoWorldPos.z >= -0.03f;

        // Check if at edge of geometry
        col = imageLoad(u_fboImg, ivec2(screenPos + screenDir));
        if (col.r > 0.0f) {
            shouldSpawn = false;
        }

        // Look for air pixel
        for (int steps = 0; steps < k_maxSteps; ++steps) {
            col = imageLoad(u_fboImg, ivec2(screenPos));
            if (col.g > 0.0f) { // we found an air pixel
                shouldSpawn = false;                

                int prevAirI = imageAtomicAdd(u_flagImg, ivec2(screenPos), 0); // TODO: replace with non-atomic operation
                // TODO: this should not be necessary
                if (prevAirI == 0) {
                    break;
                }
                --prevAirI;

                int airI = int(atomicCounterIncrement(u_airCount[u_swap]));
                if (airI >= k_maxAirPixelsSum) {
                    break;
                }

                vec4 airWorldPos = imageLoad(u_airImg, getAirTexCoord(prevAirI, WORLD_POS_OFF, 1 - u_swap));
                vec4 airVelocity = imageLoad(u_airImg, getAirTexCoord(prevAirI, MOMENTUM_OFF, 1 - u_swap));
                imageStore(u_airImg, getAirTexCoord(airI, WORLD_POS_OFF, u_swap), airWorldPos);
                imageStore(u_airImg, getAirTexCoord(airI, MOMENTUM_OFF, u_swap), airVelocity);
                mapSSBO.map[airI] = geoI;

                break;
            }
            
            screenPos += screenDir;
        }
    
        // Make a new air pixel
        if (shouldSpawn) {
            vec3 airWorldPos = geoWorldPos;
            vec3 refl = reflect(vec3(0.0f, 0.0f, -1.0f), geoNormal);
            vec3 airVelocity = refl * 10.0f;
            int airI = int(atomicCounterIncrement(u_airCount[u_swap]));
            imageStore(u_airImg, getAirTexCoord(airI, WORLD_POS_OFF, u_swap), vec4(airWorldPos, 0.0f));
            imageStore(u_airImg, getAirTexCoord(airI, MOMENTUM_OFF, u_swap), vec4(airVelocity, 0.0f));
            mapSSBO.map[airI] = geoI;
        }
    }
}