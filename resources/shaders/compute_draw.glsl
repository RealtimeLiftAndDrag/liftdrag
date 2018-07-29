#version 450
#extension GL_ARB_shader_storage_buffer_object : require

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54
#define ESTIMATEMAXOUTLINEPIXELSSUM ESTIMATEMAXOUTLINEPIXELS * (ESTIMATEMAXOUTLINEPIXELSROWS / (2 * 3))

layout (local_size_x = 1, local_size_y = 1) in;

uniform uint swap;
// local group of shaders
// compute buffers
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (rgba8, binding = 3) uniform image2D img_FBO; // framebuffer
layout (rgba32f, binding = 5) uniform image2D img_outline;	
layout (std430, binding = 0) restrict buffer ssbo_geopixels { 
    uint geo_count;
    uint test;
    uint out_count[2];
    vec4 screenratio;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugshit[4096];
} geopix;
//layout (binding = 1, offset = 0) uniform atomic_uint ac;

ivec2 loadstore_outline(uint index, int init_offset, uint swapval) { // with init_offset being 0,1 or 2  (world, momentum, tex)
    uint off = index%ESTIMATEMAXOUTLINEPIXELS;
    uint mul = index/ESTIMATEMAXOUTLINEPIXELS;
    int halfval = ESTIMATEMAXOUTLINEPIXELSROWS/2;
    return ivec2(off ,init_offset + 3*mul + swapval*halfval);
}

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
        if (work_on >= ESTIMATEMAXOUTLINEPIXELSSUM) break;
        if(work_on>=geopix.out_count[counterswap]) break;

        vec3 norm = imageLoad(img_outline,loadstore_outline(work_on, MOMENTUMOFF,counterswap)).xyz;
        vec3 worldpos = imageLoad(img_outline,loadstore_outline(work_on, WORLDPOSOFF,counterswap)).xyz;
        
        //worldpos.xy = vec2(0,0);
        vec2 w2t = worldpos.xy;
        w2t.x *= geopix.screenratio.x * (2.0f / 3.0f);
        w2t.y *= geopix.screenratio.y;
        w2t.x += geopix.screenratio.z / 2.0f;
        w2t.y += geopix.screenratio.w / 2.0f;
        
        vec4 original_color = imageLoad(img_FBO, ivec2(w2t));
        original_color.b = 1.0f;

        imageStore(img_FBO, ivec2(w2t), original_color);
        imageAtomicExchange(img_flag, ivec2(w2t), uint(work_on + 1));
    }        
}