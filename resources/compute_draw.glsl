#version 450
#extension GL_ARB_shader_storage_buffer_object : require

#define MSIZE 4096 * 8
#define ESTIMATEMAXOUTLINEPIXELS 4096

layout (local_size_x = 1, local_size_y = 1) in;

uniform uint swap;
// local group of shaders
// compute buffers
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (rgba8, binding = 3) uniform image2D img_FBO; //framebuffer
layout (std430, binding = 0) restrict buffer ssbo_geopixels { 
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
//layout (binding = 1, offset = 0) uniform atomic_uint ac;

void main() {
    int counterswap =  abs(int(swap) - 1);
    if (swap == 0) counterswap = 1;
    else counterswap = 0;

    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    uint iac = geopix.out_count[counterswap];
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    for (int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index+shadernum * ii;
        if (work_on >= ESTIMATEMAXOUTLINEPIXELS) break;
        if(work_on>=geopix.out_count[counterswap]) break;
        vec2 texpos = geopix.out_texpos[counterswap][work_on].xy;	
        vec2 norm = geopix.out_momentum[counterswap][work_on].xy;	

        float red = 0.0f, green = 0.0f;
        if (norm.y > 0.0f) red = 1.0f;
        else green = 1.0f;

        imageStore(img_FBO, ivec2(texpos), vec4(red, green, 1.0f, 0.9f));
        imageAtomicExchange(img_flag, ivec2(texpos), uint(work_on + 1));
    }        
}