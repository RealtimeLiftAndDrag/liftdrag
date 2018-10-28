#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 campos;
out vec3 vertex_pos;
out vec2 vertex_tex;

void main()
{
	vec4 tpos = M * vec4(vertPos, 1.0);
	vertex_pos = tpos.xyz;
	vertex_tex = vertTex;
	gl_Position = P * V * tpos;
}
