#version 410 core

uniform float time;

out vec4 color;
in vec3 vertex_pos;
uniform vec3 campos;


void main()
{
	color.rgb = vec3(0.0, 0.4, 0.8);

	vec3 lightSource = vec3(0.0f, cos(time), cos(time));
	vec3 distance = normalize(campos - lightSource);
	float diffuse = dot(vec3(0.0f, 1.0f, 1.0f), distance);
	color.rgb *= diffuse * 0.3;


	color.a = 0.7f;
}
