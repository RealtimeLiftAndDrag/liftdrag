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

const int k_estMaxOutlinePixels = 16384;
const int k_maxOutlinePixelsRows = 27 * 2;
const int k_maxOutlinePixelsSum = k_estMaxOutlinePixels * (k_maxOutlinePixelsRows / 2 / 3);

const int k_invocCount = 1024;

const int k_maxSteps = 50;

// Uniforms --------------------------------------------------------------------

uniform int u_swap;

layout (   r32i, binding = 2) uniform iimage2D u_flagImg;									
layout (  rgba8, binding = 3) uniform  image2D u_fboImg;	
layout (rgba32f, binding = 4) uniform  image2D u_geoImg;	
layout (rgba32f, binding = 5) uniform  image2D u_outlineImg;

layout (std430, binding = 0) restrict buffer SSBO {
    int geoCount;
    int test; // necessary for padding
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

ivec2 getOutlineTexCoord(int index, int offset, int swap) { // with offset being 0, 1, or 2  (world, momentum, tex)
    return ivec2(
        index % k_estMaxOutlinePixels,
        offset + 3 * (index / k_estMaxOutlinePixels) + swap * (k_maxOutlinePixelsRows / 2)
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
    int invocWorkload = (ssbo.geoCount + k_invocCount - 1) / k_invocCount;    
    for (int ii = 0; ii < invocWorkload; ++ii) {
    
        int workI = invocI + (k_invocCount * ii);
        if (workI >= ssbo.geoCount || workI >= k_maxGeoPixelsSum) {
            break;
        }

        vec3 geoWorldPos = imageLoad(u_geoImg, getGeoTexCoord(workI, WORLD_POS_OFF)).xyz;
        vec3 geoNormal = imageLoad(u_geoImg, getGeoTexCoord(workI, MOMENTUM_OFF)).xyz;

        // TODO: find better way to deal with this
        if (abs(geoNormal.z) > 0.99f) {
            continue;
        }

        vec2 screenPos = worldToScreen(geoWorldPos);

        vec2 screenDir = normalize(geoNormal.xy);

        bool canSpawn = true;
        for (int steps = 0; steps < k_maxSteps; ++steps) {
            vec4 col = imageLoad(u_fboImg, ivec2(screenPos));
            if (col.b > 0.0f) { // we found an outline pixel  
                canSpawn = false;
                int index = imageAtomicAdd(u_flagImg, ivec2(screenPos), 0); // TODO: replace with non-atomic operation
                if (index == 0) {
                    break;
                }
                index--;
                    
                vec3 outlineWorldPos = imageLoad(u_outlineImg, getOutlineTexCoord(index, WORLD_POS_OFF, u_swap)).xyz;

                vec3 backforceDir = outlineWorldPos - geoWorldPos;

                //float force = length(backforceDir);
                //backforceDir=normalize(backforceDir);
                //force=pow(force-0.08,0.7);
                //if(force<0)force=0;

                ivec2 velocityTexCoord = getOutlineTexCoord(index, MOMENTUM_OFF, u_swap);
                vec3 velocity = imageLoad(u_outlineImg, velocityTexCoord).xyz;				
                velocity.xy += backforceDir.xy; //* force;				
                imageStore(u_outlineImg, velocityTexCoord, vec4(velocity, 0.0f));
                
                
                float liftforce = backforceDir.y * 1e6; // TODO: currently linear
                int i_liftforce = int(liftforce);				  
                atomicAdd(ssbo.force.x, int(round(backforceDir.y * 1.0e6f)));
            
                break;
            }
           
            if (col.r > 0.0f) { // we found a geometry pixel
                vec2 nextScreenPos = floor(screenPos) + 0.5f + screenDir;
                vec4 nextCol = imageLoad(u_fboImg, ivec2(nextScreenPos));
                if (nextCol.r > 0.0f) {
                    canSpawn = false;
                    break;
                }
            }
            
            screenPos += screenDir;
        }
    
        if (canSpawn && geoNormal.z > 0.0f) { // Make a new outline
            vec3 outlineWorldPos = geoWorldPos;
            // TODO: how far from geometry should new outline be?
            outlineWorldPos.xy += screenToWorldDir(geoNormal.xy); // TODO: should this be reflected about the normal instead? 
            int arrayI = atomicAdd(ssbo.outlineCount[u_swap], 1);
            imageStore(u_outlineImg, getOutlineTexCoord(arrayI, WORLD_POS_OFF, u_swap), vec4(outlineWorldPos, 0.0f));
            imageStore(u_outlineImg, getOutlineTexCoord(arrayI, MOMENTUM_OFF, u_swap), vec4(geoNormal, 0.0f)); // TODO: should this be reflected about the normal instead? 
            //imageStore(u_outlineImg, getOutlineTexCoord(arrayI, TEX_POS_OFF, u_swap), vec4(screenPos, 0.0f, 0.0f));
            

        }
    }
}