#version 450 core

#define MAX_GEO_PIXELS 32768
#define MAX_AIR_PIXELS 32768

#define MAX_GEO_PER_AIR 3



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

struct AirGeoMapElement {
    int geoCount;
    int geoIndices[MAX_GEO_PER_AIR];
};

// Constants -------------------------------------------------------------------

const int k_invocCount = 1024;

const int k_maxSteps = 50;

const bool k_distinguishActivePixels = true; // Makes certain "active" pixels brigher for visual clarity, but lowers performance


// Uniforms --------------------------------------------------------------------

layout (binding = 0, rgba8) uniform image2D u_fboImg;
layout (binding = 3, r32i) uniform iimage2D u_flagImg;
layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int u_swap;
    int u_geoCount;
    coherent int u_airCount[2]; // TODO: does this need to be coherent?
    ivec2 u_screenSize;
    vec2 u_screenAspectFactor;
    float u_sliceSize;
    float u_windSpeed;
    float u_dt;
    int _0; // padding
    ivec4 u_momentum;
    ivec4 u_force;
    ivec4 u_dragForce;
    ivec4 u_dragMomentum;
};

// Done this way because having a lot of large static sized arrays makes shader compilation super slow for some reason
layout (binding = 1, std430) buffer GeoPixels { // TODO: should be restrict?
    GeoPixel u_geoPixels[];
};
layout (binding = 2, std430) buffer AirPixels { // TODO: should be restrict?
    AirPixel u_airPixels[];
};
layout (binding = 3, std430) buffer AirGeoMap { // TODO: should be restrict?
    coherent AirGeoMapElement u_airGeoMap[]; // TODO: does this need to be coherent?
};

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= u_screenAspectFactor; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= vec2(u_screenSize); // scale to texture space
    return screenPos;
}

void main() {
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (u_geoCount + k_invocCount - 1) / k_invocCount;    
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int geoI = invocI + (k_invocCount * ii);
        if (geoI >= u_geoCount) {
            return;
        }

        vec3 geoWorldPos = u_geoPixels[geoI].worldPos.xyz;
        vec3 geoNormal = u_geoPixels[geoI].normal.xyz;

        ivec2 geoTexCoord = ivec2(worldToScreen(geoWorldPos));

        // TODO: magic numbers
        bool shouldSpawn = geoNormal.z >= 0.01f && geoNormal.z <= 0.99f;

        // Look for existing air pixel
        vec2 screenPos = vec2(geoTexCoord) + 0.5f;
        vec2 screenDir = normalize(geoNormal.xy);
        for (int steps = 0; steps < k_maxSteps; ++steps) {
            vec4 color = imageLoad(u_fboImg, ivec2(screenPos));
            if (color.g != 0.0f) { // we found an air pixel
                int airI = imageLoad(u_flagImg, ivec2(screenPos)).x;               
                if (airI == 0) { // TODO: this should not be necessary, just here for sanity
                    continue;
                }
                --airI;

                int mapI = atomicAdd(u_airGeoMap[airI].geoCount, 1);
                if (mapI >= MAX_GEO_PER_AIR) {
                    continue;
                }
                u_airGeoMap[airI].geoIndices[mapI] = geoI;

                shouldSpawn = false;   
                break;
            }
            
            screenPos += screenDir;
        }
    
        // Make a new air pixel
        if (shouldSpawn) {
            int airI = atomicAdd(u_airCount[u_swap], 1);
            if (airI >= MAX_AIR_PIXELS) {
                continue;
            }

            u_airGeoMap[airI].geoCount = 1;
            u_airGeoMap[airI].geoIndices[0] = geoI;

            vec3 airWorldPos = geoWorldPos;
            vec3 refl = reflect(vec3(0.0f, 0.0f, -1.0f), geoNormal);
            vec3 airVelocity = refl * u_windSpeed;

            u_airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos = vec4(airWorldPos, 0.0f);
            u_airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity = vec4(airVelocity, 0.0f);

            // Draw to fbo to avoid another draw later
            vec4 color = imageLoad(u_fboImg, geoTexCoord);
            color.g = 0.33f;
            imageStore(u_fboImg, geoTexCoord, color);
        }

        // Color active geo pixels more brightly
        if (k_distinguishActivePixels) {
            vec4 color = imageLoad(u_fboImg, geoTexCoord);
            color.r = 1.0f;
            imageStore(u_fboImg, geoTexCoord, color);

            ivec2 sideTexCoord = ivec2(worldToScreen(vec3(-geoWorldPos.z, geoWorldPos.y, 0)));
            color = imageLoad(u_sideImg, sideTexCoord);
            color.r = 1.0f;
            imageStore(u_sideImg, sideTexCoord, color);
        }
    }
}