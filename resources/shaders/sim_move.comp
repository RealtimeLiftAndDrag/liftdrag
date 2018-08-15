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

const bool k_distinguishActivePixels = true; // Makes certain "active" pixels brigher for visual clarity, but lowers performance

// Uniforms --------------------------------------------------------------------

layout (binding = 0, rgba8) uniform image2D u_fboImg;
layout (binding = 4, rgba8) uniform image2D u_sideImg;

layout (binding = 0, std430) restrict buffer SSBO {
    int u_swap;
    int u_geoCount;
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
    AirGeoMapElement u_airGeoMap[];
};

// Functions -------------------------------------------------------------------

vec2 worldToScreen(vec3 world) {
    vec2 screenPos = world.xy;
    screenPos *= u_screenAspectFactor; // compensate for aspect ratio
    screenPos = screenPos * 0.5f + 0.5f; // center
    screenPos *= vec2(u_screenSize); // scale to texture space
    return screenPos;
}

vec2 screenToWorldDir(vec2 screenDir) {
    return screenDir / (float(min(u_screenSize.x, u_screenSize.y)) * 0.5f);
}

void main() {
    int invocI = int(gl_GlobalInvocationID.x);
    int invocWorkload = (u_airCount[u_swap] + k_invocCount - 1) / k_invocCount;
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int airI = invocI + (k_invocCount * ii);
        if (airI >= u_airCount[u_swap]) {
            return;
        }

        vec3 airWorldPos = u_airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos.xyz;
        vec3 airVelocity = u_airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity.xyz;

        // Calculate backforce
        vec2 backForce = vec2(0.0f);
        float lift = 0.0f;
        float drag = 0.0f;

        int geoCount = u_airGeoMap[airI].geoCount;
        // For each associated geo pixel, update backforce, lift, and drag
        // TODO: actually take multiple geo pixels into account
        for (int mapI = 0; mapI < geoCount; ++mapI) {
            int geoI = u_airGeoMap[airI].geoIndices[mapI];

            vec3 geoWorldPos = u_geoPixels[geoI].worldPos.xyz;
            vec3 geoNormal = u_geoPixels[geoI].normal.xyz;
            float dist = distance(airWorldPos.xy, geoWorldPos.xy);
            float adjustedDist = dist * 10.0f;
            backForce = normalize(-geoNormal.xy) * adjustedDist * adjustedDist;

            //float force = length(backforceDir);
            //backforceDir=normalize(backforceDir);
            //force=pow(force-0.08,0.7);
            //if(force<0)force=0;
                
            // Calculate lift     
            float liftForce = backForce.y * 1e6; // TODO: currently linear			  
            atomicAdd(u_force.x, int(round(liftForce)));

            // Calculate drag
            float massDensity = 1.0f;
            float flowVelocity = 1.0f;
            float area = screenToWorldDir(vec2(1.0f, 0.0f)).x;
            area = area * area;
            float dragC = 1.0f;
            float dragForce = 0.5f * massDensity * flowVelocity * flowVelocity * dragC * area * max(geoNormal.z, 0.0f);
            atomicAdd(u_dragForce.x, int(round(dragForce * 1e6)));

            // Color active air pixels more brightly
            if (k_distinguishActivePixels) {
                ivec2 texCoord = ivec2(worldToScreen(airWorldPos));
                vec4 color = imageLoad(u_fboImg, texCoord);
                color.g = 1.0f;
                imageStore(u_fboImg, texCoord, color);

                ivec2 sideTexCoord = ivec2(worldToScreen(vec3(-airWorldPos.z, airWorldPos.y, 0)));
                color = imageLoad(u_sideImg, sideTexCoord);
                color.g = 1.0f;
                imageStore(u_sideImg, sideTexCoord, color);
            }
        }

        // Update velocity
        airVelocity.xy += backForce; //* force;
        airVelocity.z = -u_windSpeed;
        airVelocity = normalize(airVelocity) * u_windSpeed;

        // Update location
        airWorldPos.xy += airVelocity.xy * u_dt;
        airWorldPos.z -= u_sliceSize;

        u_airPixels[airI + u_swap * MAX_AIR_PIXELS].worldPos = vec4(airWorldPos, 0.0f);
        u_airPixels[airI + u_swap * MAX_AIR_PIXELS].velocity = vec4(airVelocity, 0.0f);
    }        
}