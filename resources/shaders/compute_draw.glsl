#version 450
#extension GL_ARB_shader_storage_buffer_object : require

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54
#define ESTIMATEMAXOUTLINEPIXELSSUM ESTIMATEMAXOUTLINEPIXELS * (ESTIMATEMAXOUTLINEPIXELSROWS / (2 * 3))

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / (2*3))



layout (local_size_x = 1, local_size_y = 1) in;

uniform uint swap;
uniform uint slice;
// local group of shaders
// compute buffers
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (rgba8, binding = 3) uniform image2D img_FBO; // framebuffer
layout (rgba32f, binding = 4) uniform image2D img_geo;	
layout (rgba8, binding = 6) uniform image2D img_geo_side;	
layout (rgba32f, binding = 5) uniform image2D img_outline;	
layout (std430, binding = 0) restrict buffer ssbo_geopixels { 
    uint geo_count;
    uint test;
    uint out_count[2];
    vec4 screenSpec;
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

ivec2 load_geo(uint index,int init_offset) { // with init_offset being 0, 1, or 2 (world, momentum, tex)
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
    int counterswap =  abs(int(swap) - 1);
    if (swap == 0) counterswap = 1;
    else counterswap = 0;

    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    uint iac = geopix.out_count[counterswap];
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    for (int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index + (shadernum * ii);
        if (work_on >= ESTIMATEMAXOUTLINEPIXELSSUM) break;
        if(work_on >= geopix.out_count[counterswap]) break;

        //vec3 norm = imageLoad(img_outline,loadstore_outline(work_on, MOMENTUMOFF,counterswap)).xyz;
        vec3 worldpos = imageLoad(img_outline,loadstore_outline(work_on, WORLDPOSOFF,counterswap)).xyz;
		vec3 geo_worldpos = imageLoad(img_geo, load_geo(work_on, WORLDPOSOFF)).xyz;
        
        vec2 texPos = world_to_screen(worldpos);
        
        vec4 original_color = imageLoad(img_FBO, ivec2(texPos));
        original_color.b = 1.0f;

		vec2 sideTexPos = world_to_screen(vec3((worldpos.z - 0.5) * 4, worldpos.y * 4, 0));
		vec2 sideGeoTexPos = world_to_screen(vec3((geo_worldpos.z - 0.5) * 4, geo_worldpos.y * 4, 0));
//		sideTexPos.x = slice;
//		sideGeoTexPos.x = slice;
        imageStore(img_FBO, ivec2(texPos), original_color);
		if(worldpos.x < -.1 && worldpos.x > -.2){
			imageStore(img_geo_side, ivec2(sideTexPos), vec4(0,1,1,1));
			imageStore(img_geo_side, ivec2(sideGeoTexPos), vec4(1, 1, 0, 1));
		}
        imageAtomicExchange(img_flag, ivec2(texPos), uint(work_on + 1));
    }        
}