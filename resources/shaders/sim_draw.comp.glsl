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

const vec3 k_sideGeoColor = vec3(1.0f, 0.0f, 1.0f);
const vec3 k_sideAirColor = vec3(0.0f, 1.0f, 0.0f);

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (binding = 0,   rgba8) uniform  image2D u_fboImg;
layout (binding = 1,    r32i) uniform iimage2D u_flagImg;
layout (binding = 4,   rgba8) uniform  image2D u_sideImg;

layout (binding = 0, offset = 0) uniform atomic_uint u_geoCount;
layout (binding = 0, offset = 4) uniform atomic_uint u_airCount[2];

layout (binding = 0, std430) restrict buffer SSBO {
    vec4 screenSpec;
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
            return;
        }
        int geoI = airGeoMap[airI];

        vec3 geoWorldPos = geoPixels[geoI].worldPos.xyz;
        vec3 geoNormal = geoPixels[geoI].normal.xyz;
        vec3 airWorldPos = airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos.xyz;
        vec3 airVelocity = airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity.xyz;
        
        ivec2 airScreenPos = ivec2(worldToScreen(airWorldPos));
        ivec2 geoSideTexPos = ivec2(worldToScreen(vec3(-geoWorldPos.z, geoWorldPos.y, 0)));
        ivec2 airSideTexPos = ivec2(worldToScreen(vec3(-airWorldPos.z, airWorldPos.y, 0)));
        
        vec4 color = imageLoad(u_fboImg, airScreenPos);
        color.g = 1.0f;

        imageStore(u_fboImg, airScreenPos, color);        
        imageStore(u_sideImg, geoSideTexPos, vec4(k_sideGeoColor, 1.0f));
        imageStore(u_sideImg, airSideTexPos, vec4(k_sideAirColor, 1.0f));
        imageAtomicExchange(u_flagImg, airScreenPos, airI + 1);
    }        
}