#version 410 core

layout(triangles, fractional_even_spacing, cw) in;	//equal_spacing
in vec2 TE_vertex_tex[];
uniform mat4 P;
uniform mat4 V;
uniform vec3 camoff;
uniform vec3 campos;
uniform float time;

out vec2 vertex_tex;
out vec3 vertex_norm;
out vec3 vertex_tan;
out vec3 vertex_bi;
out vec3 vertex_pos;
out float colorO;

void main() {
	vec4 pos = (gl_TessCoord.x * gl_in[0].gl_Position +
		gl_TessCoord.y * gl_in[1].gl_Position +
		gl_TessCoord.z * gl_in[2].gl_Position);

	vec2 Tex = (gl_TessCoord.x * TE_vertex_tex[0] +
		gl_TessCoord.y * TE_vertex_tex[1] +
		gl_TessCoord.z * TE_vertex_tex[2]);

	//float height = getHeight(pos.xyz);

	//pos.y = height;

	/*vertex_norm = calculateNormal(pos.xyz);
	vertex_tan = calculateTan(vertex_norm);
	vertex_bi = calculateBinorm(vertex_norm);*/
	vertex_pos = pos.xyz;

	vertex_tex = Tex;
	gl_Position = P * V * pos;

}