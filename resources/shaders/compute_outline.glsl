#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#define STEPMAX 50

#define WORLDPOSOFF 0
#define MOMENTUMOFF 1

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXGEOPIXELSSUM ESTIMATEMAXGEOPIXELS * (ESTIMATEMAXGEOPIXELSROWS / (2 * 3))

#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54 // with half half 27

#define DEBUG_SIZE 4096

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint u_swap;

layout (   r32i, binding = 2) uniform iimage2D u_flagImg;									
layout (  rgba8, binding = 3) uniform  image2D u_fboImg;	
layout (rgba32f, binding = 4) uniform  image2D u_geoImg;	
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
    texPos *= ssbo.screenSpec.zw; //changes to be a square in texture space
    texPos += 1.0f; //centers
    texPos *= 0.5f;
    texPos *= ssbo.screenSpec.xy; //change range to a centered box in texture space
    return texPos;
}


void main() {
    int index = int(gl_GlobalInvocationID.x);
    int shadernum = 1024;
    int iac = int(ssbo.geometryCount);
    float f_cs_workload_per_shader = ceil(float(iac) / float(shadernum));
    int cs_workload_per_shader = int(f_cs_workload_per_shader);
    
    for(int ii = 0; ii < cs_workload_per_shader; ii++) {
        int work_on = index + shadernum * ii;
        if (work_on >= ESTIMATEMAXGEOPIXELSSUM) break;
        if (work_on >= ssbo.geometryCount) break;

        vec3 geo_worldpos = imageLoad(u_geoImg, load_geo(work_on, WORLDPOSOFF)).xyz;
        vec3 normal = imageLoad(u_geoImg, load_geo(work_on, MOMENTUMOFF )).xyz;

        vec2 texPos = world_to_screen(geo_worldpos);
    
        normal = normalize(normal);
        vec3 geo_normal = normal;

        if (abs(normal.y) < 0.01f) continue;
        // finding an possible outline / calculate density        
        
        vec2 ntexPos = texPos;
        vec4 col = vec4(0.0f);

        vec2 pixdirection = normalize(normal.xy);

        //TODO Sacriligious programming stuff going on here this is so wrong. Why would y be flipped?
        //pixdirection.y *= -1.f;
        bool pixelFound = false;
        for(int steps = 0; steps < STEPMAX; steps++) {
            col = imageLoad(u_fboImg, ivec2(ntexPos));
            if (col.b > 0.f) { // we found an outline pixel  
                pixelFound = true;
                int index = imageAtomicAdd(u_flagImg, ivec2(ntexPos), 0);
                if (index > 0) index--;					
                
                if (index < 1){					
                    break;
                }
                    
                vec3 out_worldpos = imageLoad(u_outlineImg, loadstore_outline(index, WORLDPOSOFF, u_swap)).xyz;

                vec3 backforce_direction = out_worldpos - geo_worldpos;
                backforce_direction *= 1; //modifier constant

                //float force = length(backforce_direction);
                //backforce_direction=normalize(backforce_direction);
                //force=pow(force-0.08,0.7);
                //if(force<0)force=0;

                vec3 momentum = imageLoad(u_outlineImg,loadstore_outline(index, MOMENTUMOFF, u_swap)).xyz;				
                momentum.xy += backforce_direction.xy; //* force;				
                imageStore(u_outlineImg, loadstore_outline(index, MOMENTUMOFF, u_swap), vec4(momentum, 0.0f));
                
                
                float liftforce = backforce_direction.y * 1e6; //pow(backforce_direction.y, 3) * 1e6;
                int i_liftforce = int(liftforce);				  
                atomicAdd(ssbo.force.x, i_liftforce);
            
                break;
            }
           
            if (col.r > 0.f) {
                vec2 nextpos = floor(ntexPos) + vec2(0.5, 0.5) + pixdirection;
                vec4 nextcol = imageLoad(u_fboImg, ivec2(nextpos) );
                if(nextcol.r > 0.f) {
                    pixelFound = true;
                    break;
                }
            }
            
            ntexPos += pixdirection;
        }
    
        if (!pixelFound && normal.z < 0.0f) { //make a new outline
            //atomicAdd(ssbo.debugShit[0].w, 1);
            // ntexPos = texPos - normal.xy * 20.5f;

            vec2 world_normal = normal.xy;// * .5f;

            world_normal.x /= ssbo.screenSpec.x / 2.f;
            world_normal.y /= ssbo.screenSpec.y / 2.f;

            vec3 out_worldpos = geo_worldpos;
            out_worldpos.xy += (world_normal.xy * 0.1);
            out_worldpos.z = geo_worldpos.z;
            uint current_array_pos = atomicAdd(ssbo.outlineCount[u_swap], 1);
            //imageStore(u_outlineImg, loadstore_outline(current_array_pos, texPosOFF, u_swap), vec4(ntexPos, 0.0f, 0.0f));   
            imageStore(u_outlineImg, loadstore_outline(current_array_pos, MOMENTUMOFF, u_swap), vec4(normal, 0.0f));
            imageStore(u_outlineImg, loadstore_outline(current_array_pos, WORLDPOSOFF, u_swap), vec4(out_worldpos, 0.0f));     
            

        }
    }
}