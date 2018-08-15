#version 450 core

#define MAX_GEO_PIXELS 32768
#define MAX_AIR_PIXELS 32768

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

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

layout (binding = 0, rgba8) uniform image2D u_fboImg;
layout (binding = 1, rgba32f) uniform image2D u_fboPosImg;
layout (binding = 2, rgba32f) uniform image2D u_fboNormImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int u_swap;
    coherent int u_geoCount;
    int u_airCount[2];
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
    int u_airGeoMap[];
};

ivec2 getPixelDelta(vec2 dir) {
    vec2 signs = sign(dir);
    vec2 mags = dir * signs;
    if (mags.y >= mags.x) {
        return ivec2(0, int(signs.y));
    }
    else {
        return ivec2(int(signs.x), 0);
    }
}

// Functions -------------------------------------------------------------------

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    if (texCoord.x >= u_screenSize.x || texCoord.y >= u_screenSize.y) {
        return;
    }

    vec4 color = imageLoad(u_fboImg, texCoord);
    if (color.r == 0.0f) {
        return;
    }

    vec4 geoWorldPos = imageLoad(u_fboPosImg, texCoord);
    vec4 geoNormal = imageLoad(u_fboNormImg, texCoord);

    ivec2 nextTexCoord = texCoord + getPixelDelta(geoNormal.xy);
    vec4 nextColor = imageLoad(u_fboImg, nextTexCoord);
    if (nextColor.r != 0.0f) {
        return;
    }

    int geoI = atomicAdd(u_geoCount, 1);
    if (geoI >= MAX_GEO_PIXELS) {
        return;
    }

    u_geoPixels[geoI].worldPos = geoWorldPos;
    u_geoPixels[geoI].normal = geoNormal;
}