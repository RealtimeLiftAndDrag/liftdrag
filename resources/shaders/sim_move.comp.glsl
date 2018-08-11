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
const int k_maxAirPixelsSum = k_maxAirPixels * (k_maxAirPixelsRows / 2 / 3);

const int k_invocCount = 1024;

const float k_windSpeed = 1.0f;
const float k_sliceSize = 0.025f;

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
    int airCount = int(atomicCounter(u_airCount[u_swap]));
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (airCount + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int airI = invocI + (k_invocCount * ii);
        if (airI >= airCount || airI >= k_maxAirPixelsSum) {
            break;
        }

        ivec2 airWorldPosTexCoord = getAirTexCoord(airI, WORLD_POS_OFF, u_swap);
        ivec2 airVelocityTexCoord = getAirTexCoord(airI, MOMENTUM_OFF, u_swap);
        vec3 airWorldPos = imageLoad(u_airImg, airWorldPosTexCoord).xyz;
        vec3 airVelocity = imageLoad(u_airImg, airVelocityTexCoord).xyz;

        int geoI = mapSSBO.map[airI];
        vec3 geoWorldPos = imageLoad(u_geoImg, getGeoTexCoord(geoI, WORLD_POS_OFF)).xyz;
        vec3 geoNormal = imageLoad(u_geoImg, getGeoTexCoord(geoI, MOMENTUM_OFF)).xyz;
        vec2 backForce = normalize(-geoNormal.xy) * distance(airWorldPos.xy, geoWorldPos.xy) * 2.0f;

        //float force = length(backforceDir);
        //backforceDir=normalize(backforceDir);
        //force=pow(force-0.08,0.7);
        //if(force<0)force=0;
                
        // Calculate lift     
        float liftForce = backForce.y * 1e6; // TODO: currently linear			  
        atomicAdd(ssbo.force.x, int(round(liftForce)));

        // Update velocity
        // TODO: this is absurd
        airVelocity.xy += backForce; //* force;
        //airVelocity.x = 0.0f;
        vec3 dir = normalize(airVelocity);
        dir.z = -1.0f;
        dir = normalize(dir);
        airVelocity = dir * 10.0f; // TODO: magic constant

        // Update location
        airWorldPos.xy += screenToWorldDir(airVelocity.xy); // TODO: right now a velocity of 1 corresponds to moving 1 pixel. Is this right?
        airWorldPos.z -= k_sliceSize;

        //vec2 screenPos = floor(worldToScreen(airWorldPos)) + 0.5f;

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
        //    vec3 dir_geo_out = airWorldPos - geo_worldpos;
        //    if (dot(dir_geo_out.xy, geo_normal) < 0) {
        //        continue;
        //    }
        //}
        //
        //if (col.g > 0.0f) {
        //    continue; // merge!!!
        //}

        imageStore(u_airImg, airWorldPosTexCoord, vec4(airWorldPos, 0.0f)); 
        imageStore(u_airImg, airVelocityTexCoord, vec4(airVelocity, 0.0f));
    }        
}