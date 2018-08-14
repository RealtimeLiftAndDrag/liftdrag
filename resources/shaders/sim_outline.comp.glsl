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

const int k_maxSteps = 50;


// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0, rgba8) uniform image2D u_fboImg;
layout (binding = 3, r32i) uniform iimage2D u_flagImg;
layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int geoCount;
    coherent int airCount[2];
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

bool addAir(vec3 worldPos, vec3 velocity, int geoI) {
    int airI = atomicAdd(ssbo.airCount[u_swap], 1);
    if (airI >= MAX_AIR_PIXELS) {
        return false;
    }

    airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos = vec4(worldPos, 0.0f);
    airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity = vec4(velocity, 0.0f);
    airGeoMap[airI] = geoI;

    return true;
}

void main() {
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (ssbo.geoCount + k_invocCount - 1) / k_invocCount;    
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int geoI = invocI + (k_invocCount * ii);
        if (geoI >= ssbo.geoCount) {
            return;
        }

        vec3 geoWorldPos = geoPixels[geoI].worldPos.xyz;
        vec3 geoNormal = geoPixels[geoI].normal.xyz;

        ivec2 geoTexCoord = ivec2(worldToScreen(geoWorldPos));

        // Color geo pixels
        vec4 color = imageLoad(u_fboImg, geoTexCoord);
        color.r = 1.0f;
        imageStore(u_fboImg, geoTexCoord, color);

        // Color geo pixels in side view
        ivec2 sideTexCoord = ivec2(worldToScreen(vec3(-geoWorldPos.z, geoWorldPos.y, 0)));
        color = imageLoad(u_sideImg, sideTexCoord);
        color.r = 1.0f;
        imageStore(u_sideImg, sideTexCoord, color);

        // TODO: magic numbers
        bool shouldSpawn = geoNormal.z >= 0.01f && geoNormal.z <= 0.99f;

        // Look for existing air pixel
        vec2 screenPos = vec2(geoTexCoord) + 0.5f;
        vec2 screenDir = normalize(geoNormal.xy);
        for (int steps = 0; steps < k_maxSteps; ++steps) {
            vec4 color = imageLoad(u_fboImg, ivec2(screenPos));
            if (color.g > 0.0f) { // we found an air pixel
                int prevAirI = imageLoad(u_flagImg, ivec2(screenPos)).x;               
                if (prevAirI == 0) { // TODO: this should not be necessary, just here for sanity
                    continue;
                }
                --prevAirI;
                
                vec3 airWorldPos = airPixels[prevAirI + (1 - u_swap) * MAX_AIR_PIXELS].worldPos.xyz;
                vec3 airVelocity = airPixels[prevAirI + (1 - u_swap) * MAX_AIR_PIXELS].velocity.xyz;
                if (!addAir(airWorldPos, airVelocity, geoI)) {
                    return;
                }

                shouldSpawn = false;   
                break;
            }
            
            screenPos += screenDir;
        }
    
        // Make a new air pixel
        if (shouldSpawn) {
            vec3 airWorldPos = geoWorldPos;
            vec3 refl = reflect(vec3(0.0f, 0.0f, -1.0f), geoNormal);
            vec3 airVelocity = refl * 10.0f;
            if (!addAir(airWorldPos, airVelocity, geoI)) {
                return;
            }

            // Draw to fbo to avoid another draw later
            vec4 color = imageLoad(u_fboImg, geoTexCoord);
            color.g = 1.0f;
            imageStore(u_fboImg, geoTexCoord, color);
        }
    }
}