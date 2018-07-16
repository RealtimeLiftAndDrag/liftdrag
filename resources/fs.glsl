#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require

#define MSIZE 4096 * 8
#define ESTIMATEMAXOUTLINEPIXELS 4096

in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;

out vec4 color;

uniform vec3 campos;
uniform sampler2D tex;
uniform sampler2D tex2;
//compute buffers
layout (rgba32f, binding = 4) uniform image2D img_geo;	
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (std430, binding=0) restrict buffer ssbo_geopixels { 
    uint geo_count;
    uint test;
    uint out_count[2];
    vec4 momentum;
    vec4 force;
    vec4 out_worldpos[2][ESTIMATEMAXOUTLINEPIXELS];
    vec4 out_momentum[2][ESTIMATEMAXOUTLINEPIXELS];
    vec4 out_texpos[2][ESTIMATEMAXOUTLINEPIXELS];
    ivec4 debugshit[4096];
} geopix;
layout (binding = 1, offset = 0) uniform atomic_uint ac;

void main() {
    color.rgb = vec3(1.0f, 0.0f, 0.0f);
    color.a = 1.0f;
    //geopix.constants.x = 66;

    uint oldval = imageAtomicExchange(img_flag, ivec2(gl_FragCoord.xy), uint(1));
    if (oldval == 0) {
        //uint counter = atomicCounterIncrement(ac);
        //geopix.constants.y = int(counter);
        uint current_array_pos = atomicAdd(geopix.geo_count, 1);
        //if (counter<MSIZE)
        //    geopix.worldpos[counter].xyz = vertex_pos;
        if (current_array_pos < MSIZE) {
            imageStore(img_geo, ivec2(current_array_pos, 0), vec4(vertex_pos, 0.0f));
            imageStore(img_geo, ivec2(current_array_pos, 1), vec4(gl_FragCoord.xy, current_array_pos, 0.0f));
            imageStore(img_geo, ivec2(current_array_pos, 2), vec4(vertex_normal, 0.0f));
        }		
        //outlinecount = current_array_pos;
    }
}