#version 410 core

layout(vertices = 3) out;

uniform vec3 campos;
uniform int meshsize;
uniform float resolution;
in vec3 vertex_pos[];
in vec2 vertex_tex[];
out vec2 TE_vertex_tex[];

float calculateTessFactLinear() {
	float df = meshsize * resolution;
	float dist = df - length(campos.xz + vertex_pos[gl_InvocationID].xz);
	dist /= df;
	dist = pow(dist, 5);

	float tessfact = dist * 5;
	tessfact = max(1, tessfact);

	return tessfact;



	/*float df = meshsize * resolution;
	float dist = df - length(campos + vertex_pos[gl_InvocationID]);
	dist /= df;
	dist = pow(dist, 20);

	float tessfact = dist * 20;
	tessfact = max(0, tessfact);

	return tessfact;*/
}

float calculateTessFactCutoff(vec3 camPosition, vec3 vertexPosition) {
	float df = meshsize * resolution;
	float dist = length(camPosition + vertexPosition);
	float tessfact;

	if (dist < 300) {
		tessfact = 32.;
	}
	else if (dist < 500) {
		tessfact = 16.;
	}
	else {
		tessfact = 8.;
	}

	return tessfact;
}


void main(void)
{ 
	float tessfact = calculateTessFactLinear();

	gl_TessLevelInner[0] = tessfact;
	gl_TessLevelInner[1] = tessfact;
	gl_TessLevelOuter[0] = tessfact;
	gl_TessLevelOuter[1] = tessfact;
	gl_TessLevelOuter[2] = tessfact;
	//gl_TessLevelOuter[3] = tessfact;

	// Everybody copies their input to their output
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	TE_vertex_tex[gl_InvocationID] = vertex_tex[gl_InvocationID];
}