#version 450 core

#define ESTIMATEMAXOUTLINEPIXELS 4096
#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / 3)

#define DEBUG_SIZE 4096

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1
#define TEXPOSOFF 2

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

layout (location = 0) out vec4 out_color;

//compute buffers
layout (r32i, binding = 2) uniform iimage2D u_flagImg;
layout (rgba32f, binding = 4) uniform image2D u_geoImg;
layout (rgba8, binding = 6) uniform image2D u_sideImg;

layout (std430, binding = 0) restrict buffer SSBO { 
    int geometryCount;
    int test;
    int outlineCount[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugShit[DEBUG_SIZE];
} ssbo;

ivec2 store_geo(int index, int init_offset) { // with init_offset being 0, 1, or 2 
    int off = index % ESTIMATEMAXGEOPIXELS;
    int mul = index / ESTIMATEMAXGEOPIXELS;
    return ivec2(off, init_offset + 3 * mul);
}

vec2 world_to_screen(vec3 world) {
    vec2 texPos = world.xy;
    texPos *= ssbo.screenSpec.zw; // compensate for aspect ratio
    texPos = texPos * 0.5f + 0.5f; // center
    texPos *= ssbo.screenSpec.xy; // scale to texture space
    return texPos;
}

void main() {
    out_color.rgb = vec3(1.0f, 0.0f, 0.0f);
    out_color.a = 1.0f;

    int oldval = imageAtomicExchange(u_flagImg, ivec2(gl_FragCoord.xy), 1);
    if (oldval == 0) {
        //int counter = atomicCounterIncrement(ac);
        //ssbo.constants.y = int(counter);
        
        int current_array_pos = atomicAdd(ssbo.geometryCount, 1);

        //if (counter<MSIZE)
        //    ssbo.worldpos[counter].xyz = vertex_pos;
        if (current_array_pos < ESTIMATEMAXGEOPIXELSSUM) {
            vec3 tmp_frag_pos = in_pos;
            tmp_frag_pos.z *= -1.f;
            imageStore(u_geoImg, store_geo(current_array_pos, WORLDPOSOFF), vec4(tmp_frag_pos, 0.0f));
            imageStore(u_geoImg, store_geo(current_array_pos, TEXPOSOFF), vec4(gl_FragCoord.xy, current_array_pos, 0.0f));
            imageStore(u_geoImg, store_geo(current_array_pos, MOMENTUMOFF), vec4(in_norm, 0.0f));
            
            int xCoordOffset = ivec2(world_to_screen(vec3(-1, 0, 0))).x;
            vec2 texPos = world_to_screen(vec3((tmp_frag_pos.z - 0.5) * 2, tmp_frag_pos.y * 2, 0));
            //texPos.x += xCoordOffset;
            //sideview
            imageStore(u_sideImg, ivec2(texPos), vec4(1.f, 0.f, 0.f, 1.f));
        }		
        //outlinecount = current_array_pos;

        //imageAtomicExchange(u_flagImg, ivec2(gl_FragCoord.xy) + ivec2(0,ssbo.screenSpec.w), current_array_pos);
    }
}