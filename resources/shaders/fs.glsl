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

uniform uint slice;

uniform sampler2D tex;
uniform sampler2D tex2;
//compute buffers
layout (rgba8, binding = 6) uniform image2D img_geo_side;	
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

vec2 world_to_screen(vec3 world) {
    vec2 texPos = world.xy;
    texPos *= geopix.screenSpec.zw; //changes to be a square in texture space
    texPos += 1.0f; //centers
    texPos *= 0.5f;
    texPos *= geopix.screenSpec.xy; //change range to a centered box in texture space
    return texPos;
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
            vec3 tmp_frag_pos = frag_pos;
            tmp_frag_pos.z *= -1.f;
            imageStore(img_geo, store_geo(current_array_pos, WORLDPOSOFF), vec4(tmp_frag_pos, 0.0f));
            imageStore(img_geo, store_geo(current_array_pos, TEXPOSOFF), vec4(gl_FragCoord.xy, current_array_pos, 0.0f));
            imageStore(img_geo, store_geo(current_array_pos, MOMENTUMOFF), vec4(frag_normal, 0.0f));
            
            uint xCoordOffset = ivec2(world_to_screen(vec3(-1, 0, 0))).x;
            vec2 texPos = world_to_screen(vec3((tmp_frag_pos.z - 0.5) * 2, tmp_frag_pos.y * 2, 0));
            //texPos.x += xCoordOffset;
            //sideview
            imageStore(img_geo_side, ivec2(texPos), vec4(1.f, 0.f, 0.f, 1.f));
        }		
        //outlinecount = current_array_pos;

        //imageAtomicExchange(img_flag, ivec2(gl_FragCoord.xy) + ivec2(0,geopix.screenSpec.w), current_array_pos);
    }
}