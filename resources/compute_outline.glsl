#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
#define MSIZE 4096*8
#define ESTIMATEMAXOUTLINEPIXELS 4096
#define STEPMAX 50
uniform uint swap;
layout(local_size_x = 1, local_size_y = 1) in;	
//local group of shaders
//compute buffers
layout(r32ui, binding = 2) uniform uimage2D img_flag;									
layout(rgba8, binding = 3) uniform image2D img_FBO;	
layout(rgba32f, binding = 4) uniform image2D img_geo;	
layout (std430, binding=0) restrict buffer ssbo_geopixels
	{ 
	uint geo_count;
	uint test;
	uint out_count[2];
	vec4 Momentum;
	vec4 Force;
	vec4 out_worldpos[2][ESTIMATEMAXOUTLINEPIXELS];
	vec4 out_momentum[2][ESTIMATEMAXOUTLINEPIXELS];
	vec4 out_texpos[2][ESTIMATEMAXOUTLINEPIXELS];
	ivec4 debugshit[4096];
	} geopix;


void main() 
	{
	int index = int(gl_GlobalInvocationID.x);
	int shadernum = 1024;
	int iac = int(geopix.geo_count);
	float f_cs_workload_per_shader = ceil(float(iac)/float(shadernum));
	int cs_workload_per_shader = int(f_cs_workload_per_shader);
	
	for(int ii=0;ii<cs_workload_per_shader;ii++)
		{
		int work_on = index+shadernum*ii;
		if(work_on>=MSIZE)	break;
		if(work_on>=geopix.geo_count)	break;

		vec3 worldpos = imageLoad(img_geo,ivec2(work_on,0)).xyz;
		vec2 texpos = imageLoad(img_geo,ivec2(work_on,1)).xy;
		vec3 normal = imageLoad(img_geo,ivec2(work_on,2)).xyz;
	
		normal=normalize(normal);
		vec3 geo_normal = normal;

		if(abs(normal.y)<0.01)break;
			//finding an possible outline / calculate density
		
		
		int steps=0;
		vec2 ntexpos = texpos;
		vec4 col = vec4(0,0,0,0);

		vec2 pixdirection = normalize(normal.xy);

		for(;steps<STEPMAX;steps++)
			{
			ntexpos -= pixdirection;
			
			col = imageLoad(img_FBO,ivec2(ntexpos));
			if(col.a < 0.95)// we hit a outline pixel
				{
				
				uint index= imageAtomicAdd(img_flag,ivec2(ntexpos),uint(0));
				if(index>0)index--;					
				
				if(index<1)
					break;
					
				//atomicAdd(geopix.test,1);
				//break;
				float distance_lift = float(steps);
				float backforce = distance_lift/700.0;
				vec3 momentum = geopix.out_momentum[swap][index].xyz;
				vec2 backforce_direction = normalize(ntexpos - texpos)*backforce;
				momentum.xy+=backforce_direction;
				geopix.out_momentum[swap][index] =  vec4(momentum,0);
				
				//not sure:
				/*
				vec3 liftforce = distance_lift*geo_normal*f;
				vec3 center_grav=...
				vec3 r = worldpos -center_grav;
				vec3 torque_pix = cross(liftforce,r);
				atomicAdd(geopix.Momentum.xyz,torque_pix);
				atomicAdd(geopix.Force.xyz,liftforce);
				*/
				break;
				}
			else if( col.r > 0.1)
				{
				break;
				}
			}
		//**************************************************	
		if(steps==STEPMAX && normal.z<0)//
			{
			ntexpos = texpos - normal.xy*5.5;
			uint current_array_pos = atomicAdd(geopix.out_count[swap],1);
			geopix.out_texpos[swap][current_array_pos] = vec4(ntexpos,0,0);
			geopix.out_momentum[swap][current_array_pos] =  vec4(normal,0);
			}
			
		
		}
		
	}