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

const int k_maxAirPixels = 16384;
const int k_maxAirPixelsRows = 27 * 2;
const int k_maxAirPixelsSum = k_estMaxAirPixels * (k_maxAirPixelsRows / 2 / 3);

const int k_invocCount = 1024;

const float k_windSpeed = 1.0f;
const float k_sliceSize = 0.025f;

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (   r32i, binding = 2) uniform iimage2D u_flagImg;									
layout (  rgba8, binding = 3) uniform  image2D u_fboImg;	
layout (rgba32f, binding = 4) uniform  image2D u_geoImg;	
layout (rgba32f, binding = 5) uniform  image2D u_airImg;

layout (std430, binding = 0) restrict buffer SSBO {
    int geoCount;
    int test; // necessary for padding
    int airCount[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugShit[DEBUG_SIZE];
} ssbo;

layout (std430, binding = 1) buffer MapSSBO { // TODO: should be restrict?
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
        index % k_maxAirPixels,
        offset + 3 * (index / k_maxAirPixels) + swap * (k_maxAirPixelsRows / 2)
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
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (ssbo.airCount[u_swap] + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int workI = invocI + (k_invocCount * ii);
        if (workI >= ssbo.airCount[u_swap] || workI >= k_maxAirPixelsSum) {
            break;
        }

        ivec2 worldPosTexCoord = getAirTexCoord(workI, WORLD_POS_OFF, u_swap);
        ivec2 velocityTexCoord = getAirTexCoord(workI, MOMENTUM_OFF, u_swap);

        vec3 worldPos = imageLoad(u_airImg, worldPosTexCoord).xyz;
        vec3 velocity = imageLoad(u_airImg, velocityTexCoord).xyz;

        // Update location
        worldPos.xy += screenToWorldDir(velocity.xy); // TODO: right now a velocity of 1 corresponds to moving 1 pixel. Is this right?
        worldPos.z -= k_sliceSize;
        vec2 screenPos = floor(worldToScreen(worldPos)) + 0.5f;

        // Update velocity
        // TODO: how exactly are we treating velocity?
        vec3 dir = normalize(velocity);
        dir.z = -1.0f;
        dir = normalize(dir);

        velocity = dir * 5.0f; // TODO: magic constant

        //vec4 col = imageLoad(u_fboImg, ivec2(screenPos));
        //if (col.r > 0.0f) {
        //    ivec2 nextPixel = ivec2(floor(screenPos) + 0.5f + normalize(dir.xy));
        //    vec4 nextcol = imageLoad(u_fboImg, nextPixel);			
        //    if (nextcol.r > 0.0f) {
        //        continue;
        //    }
        //
        //    int geo_index = imageAtomicAdd(u_flagImg, ivec2(screenPos) + ivec2(0, ssbo.screenSpec.y), 0);
        //    vec3 geo_worldpos = imageLoad(u_geoImg, getGeoTexCoord(geo_index, WORLD_POS_OFF)).xyz;
        //    vec2 geo_normal = imageLoad(u_geoImg, getGeoTexCoord(geo_index, MOMENTUM_OFF)).xy;
        //
        //    vec3 dir_geo_out = worldPos - geo_worldpos;
        //    if (dot(dir_geo_out.xy, geo_normal) < 0) {
        //        continue;
        //    }
        //}
        //
        //if (col.g > 0.0f) {
        //    continue; // merge!!!
        //}

        imageStore(u_airImg, worldPosTexCoord, vec4(worldPos, 0.0f)); 
        imageStore(u_airImg, velocityTexCoord, vec4(velocity, 0.0f));
    }        
}