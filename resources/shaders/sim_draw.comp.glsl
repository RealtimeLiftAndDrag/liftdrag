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

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0, rgba8) uniform image2D u_fboImg;
layout (binding = 3, r32i) uniform iimage2D u_flagImg;
layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int geoCount;
    int airCount[2];
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

void main() {
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (ssbo.airCount[u_swap] + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {

        int airI = invocI + (k_invocCount * ii);
        if (airI >= ssbo.airCount[u_swap]) {
            return;
        }

        vec3 worldPos = airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos.xyz;
        vec3 velocity = airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity.xyz;
        
        ivec2 texCoord = ivec2(worldToScreen(worldPos));
        ivec2 sideTexCoord = ivec2(worldToScreen(vec3(-worldPos.z, worldPos.y, 0)));
        
        // Draw to front view
        vec4 color = imageLoad(u_fboImg, texCoord);
        color.g = 1.0f;
        imageStore(u_fboImg, texCoord, color);

        // Draw to side view
        color = imageLoad(u_sideImg, sideTexCoord);
        color.g = 1.0f;
        imageStore(u_sideImg, sideTexCoord, color);

        // Store air index
        imageAtomicExchange(u_flagImg, texCoord, airI + 1);
    }        
}