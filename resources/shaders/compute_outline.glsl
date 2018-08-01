#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
#define STEPMAX 50

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / (2*3))

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54 // with half half 27

layout (local_size_x = 1, local_size_y = 1) in;

uniform uint swap;
uniform uint slice;

// local group of shaders
// compute buffers
layout (r32ui, binding = 2) uniform uimage2D img_flag;									
layout (rgba8, binding = 3) uniform image2D img_FBO;	
layout (rgba32f, binding = 4) uniform image2D img_geo;	
layout (rgba32f, binding = 5) uniform image2D img_outline;	
layout (std430, binding = 0) restrict buffer ssbo_geopixels { 
    uint geo_count;
    uint test;
    uint out_count[2];
    vec4 screenSpec;
    ivec4 momentum;
    ivec4 force;
    ivec4 debugshit[4096];
	vec4 sideview[50][2];
} geopix;

ivec2 load_geo(uint index,int init_offset) { // with init_offset being 0, 1, or 2 (world, momentum, tex)
    uint off = index % ESTIMATEMAXGEOPIXELS;
    uint mul = index / ESTIMATEMAXGEOPIXELS;
    return ivec2(off, init_offset + 3 * mul);
}

ivec2 loadstore_outline(uint index, int init_offset, uint swapval) { // with init_offset being 0, 1, or 2  (world, momentum, tex)
    uint off = index % ESTIMATEMAXOUTLINEPIXELS;
    uint mul = index / ESTIMATEMAXOUTLINEPIXELS;
    int halfval = ESTIMATEMAXOUTLINEPIXELSROWS / 2;
    return ivec2(off, init_offset + (3 * mul) + (swapval * halfval));
}

vec2 world_to_screen(vec3 world) {
	vec2 texPos = world.xy;
	texPos *= geopix.screenSpec.zw; //changes to be a square in texture space
	texPos += 1.0f; //centers
	texPos *= 0.5f;
	texPos *= geopix.screenSpec.xy; //change range to a centered box in texture space
	return texPos;
}


void drawToSideView(vec3 pos)
{
    if (pos.x > -0.2 && pos.x < 0.2)
    {
        if (pos.y < 0 && geopix.sideview[slice][0].w == 0)
        {
            geopix.sideview[slice][0].x = float(slice);
            geopix.sideview[slice][0].y = pos.y;
            geopix.sideview[slice][0].w = 1;
        }
        else if (pos.y > 0 && geopix.sideview[slice][1].w == 0)
        {
            geopix.sideview[slice][1].x = slice;
            geopix.sideview[slice][1].y = pos.y;
            geopix.sideview[slice][1].w = 1;
        }
        if (slice == 0)
        {
            geopix.sideview[49][0].w = 0;

            geopix.sideview[49][1].w = 0;
        }
        else
        {
            geopix.sideview[slice - 1][0].w = 0;
            geopix.sideview[slice - 1][1].w = 0;
        }
    }
}

void main() {
    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    int iac = int(geopix.geo_count);
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    
    for(int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index + shadernum * ii;
        if (work_on >= ESTIMATEMAXGEOPIXELSSUM) break;
        if (work_on >= geopix.geo_count) break;

        vec3 geo_worldpos = imageLoad(img_geo, load_geo(work_on, WORLDPOSOFF)).xyz;
        vec3 normal = imageLoad(img_geo, load_geo(work_on, MOMENTUMOFF )).xyz;

		vec2 texPos = world_to_screen(geo_worldpos);
    
        normal = normalize(normal);
        vec3 geo_normal = normal;

        if (abs(normal.y) < 0.01f) continue;
        // finding an possible outline / calculate density        
        
        vec2 ntexPos = texPos;
        vec4 col = vec4(0.0f);

        vec2 pixdirection = normalize(normal.xy);

		//TODO Sacriligious programming stuff going on here this is so wrong. Why would y be flipped?
		pixdirection.y *= -1.f;
		bool pixelFound = false;
        for(int steps = 0; steps < STEPMAX; steps++) {
            col = imageLoad(img_FBO, ivec2(ntexPos));
            if (col.b > 0.f) { // we found an outline pixel  
				pixelFound = true;
                uint index = imageAtomicAdd(img_flag, ivec2(ntexPos), uint(0));
                if (index > 0) index--;					
                
                if (index < 1){					
                    break;
				}
                    
                vec3 out_worldpos = imageLoad(img_outline,loadstore_outline(index, WORLDPOSOFF,swap)).xyz;

                vec3 backforce_direction = out_worldpos - geo_worldpos;
                backforce_direction *= 1; //modifier constant
                vec3 momentum = imageLoad(img_outline,loadstore_outline(index, MOMENTUMOFF,swap)).xyz;				
                momentum.xy += backforce_direction.xy;				
                imageStore(img_outline, loadstore_outline(index, MOMENTUMOFF,swap), vec4(momentum, 0.0f));
                

                float liftforce = pow(backforce_direction.y, 2) * 1e6;
                int i_liftforce = int(liftforce);				  
                atomicAdd(geopix.force.x, i_liftforce);
                drawToSideView(out_worldpos);
            
                break;
            }
           
            if (col.r > 0.f) {
                vec2 nextpos = floor(ntexPos) + vec2(0.5, 0.5) + pixdirection;
                vec4 nextcol = imageLoad(img_FBO, ivec2(nextpos) );
                if(nextcol.r > 0.f) {
					pixelFound = true;
                    break;
                }
            }
            
            ntexPos += pixdirection;
        }
    
        if (!pixelFound && normal.z < 0.0f) { //make a new outline
            atomicAdd(geopix.debugshit[0].w, 1);
            // ntexPos = texPos - normal.xy * 20.5f;

            vec2 world_normal = normal.xy;// * .5f;

            world_normal.x /= geopix.screenSpec.x / 2.f;
            world_normal.y /= geopix.screenSpec.y / 2.f;

            vec3 out_worldpos = geo_worldpos;
            out_worldpos.xy += (world_normal.xy * 0.1);

            uint current_array_pos = atomicAdd(geopix.out_count[swap], 1);
           
            //imageStore(img_outline, loadstore_outline(current_array_pos, texPosOFF, swap), vec4(ntexPos, 0.0f, 0.0f));   
            imageStore(img_outline, loadstore_outline(current_array_pos, MOMENTUMOFF, swap), vec4(normal, 0.0f));
            imageStore(img_outline, loadstore_outline(current_array_pos, WORLDPOSOFF, swap), vec4(out_worldpos, 0.0f));     
			
            drawToSideView(out_worldpos);

        }
    }
}