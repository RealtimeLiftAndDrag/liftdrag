#version 450 core

#define MAX_GEO_PIXELS 32768
#define MAX_AIR_PIXELS 32768

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

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

const float k_windSpeed = 1.0f;
const float k_sliceSize = 0.025f;

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0,   rgba8) uniform  image2D u_fboImg;
layout (binding = 1,    r32i) uniform iimage2D u_flagImg;

layout (binding = 0, offset = 0) uniform atomic_uint u_geoCount;
layout (binding = 0, offset = 4) uniform atomic_uint u_airCount[2];

layout (binding = 0, std430) restrict buffer SSBO {
    ivec2 screenSize;
    vec2 screenAspectFactor;
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

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= ssbo.screenAspectFactor; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= vec2(ssbo.screenSize); // scale to texture space
    return screenPos;
}

vec2 screenToWorldDir(vec2 screenDir) {
    return screenDir / (float(min(ssbo.screenSize.x, ssbo.screenSize.y)) * 0.5f);
}

void main() {
    int airCount = int(atomicCounter(u_airCount[u_swap]));
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (airCount + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int airI = invocI + (k_invocCount * ii);
        if (airI >= airCount) {
            return;
        }

        vec3 airWorldPos = airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos.xyz;
        vec3 airVelocity = airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity.xyz;

        int geoI = airGeoMap[airI];
        vec3 geoWorldPos = geoPixels[geoI].worldPos.xyz;
        vec3 geoNormal = geoPixels[geoI].normal.xyz;
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

        airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos = vec4(airWorldPos, 0.0f);
        airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity = vec4(airVelocity, 0.0f);
    }        
}