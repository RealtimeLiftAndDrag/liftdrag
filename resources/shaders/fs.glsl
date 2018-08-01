#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require

#define ESTIMATEMAXOUTLINEPIXELS 4096
//***************************************************************************
#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS *(ESTIMATEMAXGEOPIXELSROWS/3)


#define WORLDPOSOFF 0
#define MOMENTUMOFF 1
#define TEXPOSOFF 2

in vec3 frag_normal;
in vec3 frag_pos;
in vec2 frag_tex;

out vec4 color;

uniform sampler2D tex;
uniform sampler2D tex2;
//compute buffers
layout (rgba32f, binding = 4) uniform image2D img_geo;	
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (std430, binding = 0) restrict buffer ssbo_geopixels { 
    uint geo_count;
    uint test;
    uint out_count[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugshit[4096];
} geopix;

ivec2 store_geo(uint index, int init_offset) { // with init_offset being 0, 1, or 2 
    uint off = index % ESTIMATEMAXGEOPIXELS;
    uint mul = index / ESTIMATEMAXGEOPIXELS;
    return ivec2(off, init_offset + 3 * mul);
}

void main() {
    color.rgb = vec3(1.0f, 0.0f, 0.0f);
    //color.rgb = frag_normal;
    color.a = 1.0f;
    //geopix.constants.x = 66;

    uint oldval = imageAtomicExchange(img_flag, ivec2(gl_FragCoord.xy), uint(1));
    if (oldval == 0) {
        //uint counter = atomicCounterIncrement(ac);
        //geopix.constants.y = int(counter);
        
        uint current_array_pos = atomicAdd(geopix.geo_count, 1);

        //if (counter<MSIZE)
        //    geopix.worldpos[counter].xyz = vertex_pos;
        if (current_array_pos < ESTIMATEMAXGEOPIXELSSUM) {
            imageStore(img_geo, store_geo(current_array_pos, WORLDPOSOFF), vec4(frag_pos, 0.0f));
            imageStore(img_geo, store_geo(current_array_pos, TEXPOSOFF), vec4(gl_FragCoord.xy, current_array_pos, 0.0f));
            imageStore(img_geo, store_geo(current_array_pos, MOMENTUMOFF), vec4(frag_normal, 0.0f));
        }		
        //outlinecount = current_array_pos;

        //imageAtomicExchange(img_flag, ivec2(gl_FragCoord.xy) + ivec2(0,geopix.screenSpec.w), current_array_pos);
    }
}