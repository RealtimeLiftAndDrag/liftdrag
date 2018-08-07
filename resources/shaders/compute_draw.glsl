#version 450
#extension GL_ARB_shader_storage_buffer_object : require

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54
#define ESTIMATEMAXOUTLINEPIXELSSUM ESTIMATEMAXOUTLINEPIXELS * (ESTIMATEMAXOUTLINEPIXELSROWS / (2 * 3))

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / (2 * 3))

#define DEBUG_SIZE 4096

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint u_swap;

layout (   r32i, binding = 2) uniform iimage2D u_flagImg;									
layout (  rgba8, binding = 3) uniform  image2D u_fboImg;
layout (rgba32f, binding = 4) uniform  image2D u_geoImg;	
layout (  rgba8, binding = 6) uniform  image2D u_geoSideImg;	
layout (rgba32f, binding = 5) uniform  image2D u_outlineImg;

layout (std430, binding = 0) restrict buffer SSBO { 
    uint geometryCount;
    uint test; // necessary for padding
    uint outlineCount[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugShit[DEBUG_SIZE];
} ssbo;

ivec2 loadstore_outline(uint index, int init_offset, uint swapval) { // with init_offset being 0,1 or 2  (world, momentum, tex)
    uint off = index % ESTIMATEMAXOUTLINEPIXELS;
    uint mul = index / ESTIMATEMAXOUTLINEPIXELS;
    int halfval = ESTIMATEMAXOUTLINEPIXELSROWS / 2;
    return ivec2(off, init_offset + 3 * mul + swapval * halfval);
}

ivec2 load_geo(uint index, int init_offset) { // with init_offset being 0, 1, or 2 (world, momentum, tex)
    uint off = index % ESTIMATEMAXGEOPIXELS;
    uint mul = index / ESTIMATEMAXGEOPIXELS;
    return ivec2(off, init_offset + 3 * mul);
}

vec2 world_to_screen(vec3 world) {
    vec2 texPos = world.xy;
    texPos *= ssbo.screenSpec.zw; //changes to be a square in texture space
    texPos += 1.0f; //centers
    texPos *= 0.5f;
    texPos *= ssbo.screenSpec.xy; //change range to a centered box in texture space
    return texPos;
}

void main() {
    uint counterswap = 1 - u_swap;
    if (u_swap == 0) counterswap = 1;
    else counterswap = 0;

    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    uint iac = ssbo.outlineCount[counterswap];
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    for (int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index + (shadernum * ii);
        if (work_on >= ESTIMATEMAXOUTLINEPIXELSSUM) break;
        if(work_on >= ssbo.outlineCount[counterswap]) break;

        //vec3 norm = imageLoad(u_outlineImg, loadstore_outline(work_on, MOMENTUMOFF, counterswap)).xyz;
        vec3 worldpos = imageLoad(u_outlineImg, loadstore_outline(work_on, WORLDPOSOFF, counterswap)).xyz;
        vec3 geo_worldpos = imageLoad(u_geoImg, load_geo(work_on, WORLDPOSOFF)).xyz;
        
        vec2 texPos = world_to_screen(worldpos);
        
        vec4 original_color = imageLoad(u_fboImg, ivec2(texPos));
        original_color.b = 1.0f;

        vec2 sideTexPos = world_to_screen(vec3((worldpos.z - 0.5) * 2, worldpos.y * 2, 0));
        vec2 sideGeoTexPos = world_to_screen(vec3((geo_worldpos.z - 0.5) * 2, geo_worldpos.y * 2, 0));
//		sideTexPos.x = slice;
//		sideGeoTexPos.x = slice;
        imageStore(u_fboImg, ivec2(texPos), original_color);
        if(worldpos.x < -.1 && worldpos.x > -.2){
            imageStore(u_geoSideImg, ivec2(sideTexPos), vec4(0.0f, 1.0f, 1.0f, 1.0f));
            imageStore(u_geoSideImg, ivec2(sideGeoTexPos), vec4(1.0f, 1.0f, 0.0f, 1.0f));
        }
        imageAtomicExchange(u_flagImg, ivec2(texPos), work_on + 1);
    }        
}