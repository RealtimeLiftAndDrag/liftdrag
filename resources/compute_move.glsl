#version 450
#extension GL_ARB_shader_storage_buffer_object : require

#define MSIZE 4096 * 8
#define ESTIMATEMAXOUTLINEPIXELS 4096

layout (local_size_x = 1, local_size_y = 1) in;

uniform uint swap;
// local group of shaders
// compute buffers
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (rgba8, binding = 3) uniform image2D img_FBO;	
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
    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    uint iac = geopix.out_count[swap];
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    int counterswap =  abs(int(swap) - 1);
    if (swap == 0) counterswap = 1;
    else counterswap = 0;
    for (int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index+shadernum * ii;
        if (work_on >= ESTIMATEMAXOUTLINEPIXELS) break;
        if (work_on >= geopix.out_count[swap]) break;

        vec2 texpos = geopix.out_texpos[swap][work_on].xy;
        vec4 col = imageLoad(img_FBO,ivec2(texpos));
        if (col.r > 0.1f)
            continue;
        vec3 normal = geopix.out_momentum[swap][work_on].xyz;
        normal.z = 0.0f;
        vec3 winddirection = vec3(0.0f, 0.0f, 1.0f);
        vec3 direction_result = normalize(normal + winddirection);


        vec2 ntexpos = texpos - direction_result.xy * 8.5f;
        col = imageLoad(img_FBO, ivec2(ntexpos));
        if (col.b < 0.1f && col.g < 0.1f && col.r < 0.1f) {			
            uint current_array_pos = atomicAdd(geopix.out_count[counterswap], 1);
            if(current_array_pos >= ESTIMATEMAXOUTLINEPIXELS)
                break;
            geopix.out_texpos[counterswap][current_array_pos] = vec4(ntexpos, 0.0f, 0.0f);
            geopix.out_momentum[counterswap][current_array_pos] = vec4(direction_result, 0.0f);
        }

    }
        
}