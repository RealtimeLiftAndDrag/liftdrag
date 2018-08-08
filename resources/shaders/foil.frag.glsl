#version 450 core

#define DEBUG_SIZE 4096

#define WORLD_POS_OFF 0
#define MOMENTUM_OFF 1
#define TEX_POS_OFF 2

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;

// Constants -------------------------------------------------------------------

const int k_maxGeoPixels = 16384;
const int k_maxGeoPixelsRows = 27;
const int k_maxGeoPixelsSum = k_maxGeoPixels * (k_maxGeoPixelsRows / 3);

// Uniforms --------------------------------------------------------------------

layout (r32i, binding = 2) uniform iimage2D u_flagImg;
layout (rgba32f, binding = 4) uniform image2D u_geoImg;
layout (rgba8, binding = 6) uniform image2D u_sideImg;

layout (std430, binding = 0) restrict buffer SSBO { 
    int geoCount;
    int test;
    int outlineCount[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugShit[DEBUG_SIZE];
} ssbo;

// Functions -------------------------------------------------------------------

ivec2 getGeoTexCoord(int index, int offset) { // with offset being 0, 1, or 2 (world, momentum, tex)
    return ivec2(
        index % k_maxGeoPixels,
        offset + 3 * (index / k_maxGeoPixels)
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
    out_color.rgb = vec3(1.0f, 0.0f, 0.0f);
    out_color.a = 1.0f;

    int oldFlag = imageAtomicExchange(u_flagImg, ivec2(gl_FragCoord.xy), 1);
    if (oldFlag == 0) {        
        int arrayI = atomicAdd(ssbo.geoCount, 1);
        if (arrayI < k_maxGeoPixelsSum) {
            imageStore(u_geoImg, getGeoTexCoord(arrayI, WORLD_POS_OFF), vec4(in_pos, 0.0f));
            imageStore(u_geoImg, getGeoTexCoord(arrayI, MOMENTUM_OFF), vec4(in_norm, 0.0f));
            imageStore(u_geoImg, getGeoTexCoord(arrayI, TEX_POS_OFF), vec4(gl_FragCoord.xy, arrayI, 0.0f));
            
            //sideview
            vec2 sideTexPos = worldToScreen(vec3(in_pos.zy, 0));
            imageStore(u_sideImg, ivec2(sideTexPos), vec4(1.0f, 0.f, 0.f, 1.0f));
        }
    }
}