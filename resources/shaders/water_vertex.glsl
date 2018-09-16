#version 410 core
layout(location = 0) in vec3 vertPos;
//ayout(location = 1) in vec2 vertTex;


uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
out vec2 vertex_tex;

uniform vec3 camoff;


void main()
{
	/*vec2 texcoords = vertTex;
	float t = 1. / 100;
	texcoords -= vec2(camoff.x, camoff.z) * t;*/

	vec4 tpos = vec4(vertPos, 1.0);

	tpos.z -= camoff.z;
	tpos.x -= camoff.x;

	tpos = M * tpos;

	gl_Position = P * V * tpos;
	//vertex_tex = texcoords;
	vertex_pos = tpos.xyz;
}

