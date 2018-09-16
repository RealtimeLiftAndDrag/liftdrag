#version 410 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;
uniform float time;

uniform sampler2D dayTexSampler;
uniform sampler2D nightTexSampler;

float oscillate(float t, float min, float max) {
	float halfRange = (max - min) / 2;
	return (min + halfRange) + (sin(t + 5.4) * halfRange);
}

vec3 applyFog(vec3  rgb, float distance, float density) {
	float fogAmount = 1.0 - exp(-distance * density);
	vec3  fogColor = vec3(0.5, 0.6, 0.7);
	return mix(rgb, fogColor, fogAmount);
}

float normalizeFloat(float x, float min, float max) {
	return (x - min) / (max - min);
}

void main() {
	vec4 tcol = texture(dayTexSampler, vertex_tex);
	vec4 ncol = texture(nightTexSampler, vertex_tex);


	//color.rgb = vec3(1, 0, 0);
	color.rgb = mix(tcol.rgb, ncol.rgb, oscillate(time, 0.1f, 1.0f));
	
	//float y = normalizeFloat(vertex_tex.y, 0.0f, 0.5f);

	if (vertex_tex.y > 0.44) {
		color.rgb = mix(color.rgb, vec3(0.5, 0.6, 0.7) * 0.5, normalizeFloat(vertex_tex.y, 0.44, 0.5));
		//color.rgb = vec3(1);
	}
	 if (vertex_tex.y > 0.5) {
		color.rgb = vec3(0.5, 0.6, 0.7) * 0.5;
	}

	color.rgb = mix(color.rgb, vec3(1.0, 0.6, 0.2), oscillate(time * 2, 0.0f, 0.3f));

	//color.rgb = vec3(vertex_tex.y, 0, 0);
	//color.rgb = applyFog(color.rgb, length(campos - vertex_pos), 0.001f);
	//color.rgb = vertex_pos - campos;
	color.a = 1;
}
