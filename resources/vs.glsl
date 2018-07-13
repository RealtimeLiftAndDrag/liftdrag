#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 MR;
out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;
void main()
{
	vertex_normal = vec4(MR * vec4(vertNor,1.0)).xyz;
	//vertex_normal = vertNor;
	vec4 pos = M * (vec4(vertPos,1.0));
	gl_Position = P * V * pos;
	vertex_tex = vertTex;	
}
