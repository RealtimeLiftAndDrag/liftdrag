#version 450 core

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54
#define ESTIMATEMAXOUTLINEPIXELSSUM ESTIMATEMAXOUTLINEPIXELS * (ESTIMATEMAXOUTLINEPIXELSROWS / (2 * 3))

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / 3)

#define DEBUG_SIZE 4096

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int u_swap;

layout (   r32i, binding = 2) uniform iimage2D u_flagImg;									
layout (  rgba8, binding = 3) uniform  image2D u_fboImg;	
layout (rgba32f, binding = 4) uniform  image2D u_geoImg;	
layout (rgba32f, binding = 5) uniform  image2D u_outlineImg;

layout (std430, binding = 0) restrict buffer SSBO {
    int geometryCount;
    int test; // necessary for padding
    int outlineCount[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugShit[DEBUG_SIZE];
} ssbo;
//layout (binding = 1, offset = 0) uniform atomic_int ac;

ivec2 load_geo(int index,int init_offset) { // with init_offset being 0,1 or 2 (world, momentum, tex)
    int off = index % ESTIMATEMAXGEOPIXELS;
    int mul = index / ESTIMATEMAXGEOPIXELS;
    return ivec2(off, init_offset + 3 * mul);
}

ivec2 loadstore_outline(int index, int init_offset, int swapval) { // with init_offset being 0,1 or 2  (world, momentum, tex)
    int off = index % ESTIMATEMAXOUTLINEPIXELS;
    int mul = index / ESTIMATEMAXOUTLINEPIXELS;
    int halfval = ESTIMATEMAXOUTLINEPIXELSROWS/2;
    return ivec2(off, init_offset + 3 * mul + swapval * halfval);
}

vec2 world_to_screen(vec3 world) {
    vec2 texPos = world.xy;
    texPos *= ssbo.screenSpec.zw; // compensate for aspect ratio
    texPos = texPos * 0.5f + 0.5f; // center
    texPos *= ssbo.screenSpec.xy; // scale to texture space
    return texPos;
}

void main() {
    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    int iac = ssbo.outlineCount[u_swap];
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    int counterswap = 1 - u_swap;
    for (int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index + shadernum * ii;
        if (work_on >= ESTIMATEMAXOUTLINEPIXELSSUM) break;
        if (work_on >= ssbo.outlineCount[u_swap]) break;

        vec3 worldpos = imageLoad(u_outlineImg, loadstore_outline(work_on, WORLDPOSOFF, u_swap)).xyz;    

        vec3 vel = imageLoad(u_outlineImg, loadstore_outline(work_on, MOMENTUMOFF, u_swap)).xyz;        

        vel.z = 0.0f;
        vec3 winddirection = vec3(0.0f, 0.0f, 1.0f);
        vec3 direction_result = normalize(vel + winddirection);
      
        vec3 world_direction = direction_result * 5.5f;

        world_direction.x /= ssbo.screenSpec.x / 2.f;
        world_direction.y /= ssbo.screenSpec.y / 2.f;

        //TODO Sacriligious programming stuff going on here this is so wrong. Why would y be flipped?
        //world_direction.y *= -1.f;
        vec3 newworldpos = worldpos;
        newworldpos.xy += world_direction.xy;
        newworldpos.z += 0.01f;
        vec2 texPos = world_to_screen(newworldpos);

        vec4 col = imageLoad(u_fboImg, ivec2(texPos));

       // col = imageLoad(u_fboImg, ivec2(newtexpos));

       if (col.r > 0.5f) { //its geo!
            vec2 nextpos = texPos;	
            nextpos = floor(nextpos) + vec2(0.5f, 0.5f) + normalize(vec2(world_direction.x, -world_direction.y));
            vec4 nextcol = imageLoad(u_fboImg, ivec2(nextpos) );			
            if (nextcol.r > 0.5f) {
                continue;
            }

            int geo_index = imageAtomicAdd(u_flagImg, ivec2(texPos) + ivec2(0, ssbo.screenSpec.y), 0);
            vec3 geo_worldpos = imageLoad(u_geoImg, load_geo(geo_index, WORLDPOSOFF)).xyz;
            vec2 geo_normal = imageLoad(u_geoImg, load_geo(geo_index, MOMENTUMOFF)).xy;

            vec3 dir_geo_out = newworldpos - geo_worldpos;
            if (dot(dir_geo_out.xy, geo_normal) < 0)
                continue;
        }       
       
        if (col.b > 0.5f) 
            continue; // merge!!!

        int current_array_pos = atomicAdd(ssbo.outlineCount[counterswap], 1);
        if(current_array_pos >= ESTIMATEMAXOUTLINEPIXELSSUM)
            break;
    
        //imageStore(u_outlineImg, loadstore_outline(current_array_pos, TEXPOSOFF, counterswap), vec4(newtexpos, 0.0f, 0.0f));   
        imageStore(u_outlineImg, loadstore_outline(current_array_pos, MOMENTUMOFF, counterswap), vec4(direction_result, 0.0f));
        imageStore(u_outlineImg, loadstore_outline(current_array_pos, WORLDPOSOFF, counterswap), vec4(newworldpos, 0.0f));
    }        
}