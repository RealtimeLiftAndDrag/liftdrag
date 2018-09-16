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
out float temperature;
out float humidity;
out float colorO;

#define TERRAIN_TRANSFORM_RATE 50000

float hash(float n) {
	return fract(sin(n) * 753.5453123);
}

float snoise(vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0f - (2.0f * f));

	float n = p.x + p.y * 157.0f + 113.0f * p.z;
	return mix(mix(mix(hash(n + 0.0f), hash(n + 1.0f), f.x),
		mix(hash(n + 157.0f), hash(n + 158.0f), f.x), f.y),
		mix(mix(hash(n + 113.0f), hash(n + 114.0f), f.x),
			mix(hash(n + 270.0f), hash(n + 271.0f), f.x), f.y), f.z);
}

float noise(vec3 position, int octaves, float frequency, float persistence) {
	float total = 0.0f;
	float maxAmplitude = 0.0f;
	float amplitude = 1.0f;
	for (int i = 0; i < octaves; i++) {
		total += snoise(position * frequency) * amplitude;
		frequency *= 2.0f;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}



float oscillate(float t, float min, float max) {
	float halfRange = (max - min) / 2.0f;
	return (min + halfRange) + (cos(t / TERRAIN_TRANSFORM_RATE) * halfRange);
}

float normalizeFloat(float x, float min, float max) {
	return (x - min) / (max - min);
}

float getHeight(vec3 pos) {

	float dist = length(pos);

	int biomeOctaves = 3;
	float biomeFreq = 0.01f;
	float biomePers = 0.3f;
	float biomePreIntensity = 2;
	float biomePower = 2;
	float biomePostIntensity = 3;
	float biomeTranslate = 0.0f;

	float biomeHeight = pow(noise(pos.xyz + vec3(100.0), 2, 0.01f, 0.1f), 2) * 1;
	biomeHeight = clamp(biomeHeight, 0, 1);

	float biomeTemperature = noise(pos.xyz + vec3(100.0), 2, 0.002f, 0.1f) * 1;
	float biomeHumidity = noise(pos.xyz - vec3(100.0), 2, 0.002f, 0.1f) * 1;
	biomeTemperature = pow(biomeTemperature, 3) * 3;
	biomeHumidity = pow(biomeHumidity, 3) * 3;
	temperature = clamp(biomeTemperature, 0, 1);
	humidity = clamp(biomeHumidity, 0, 1);

	colorO = biomeHeight;

	int baseOctaves = 11;
	float baseFreq = 0.05;
	float basePers = mix(0.2, 0.2f, biomeHeight);
	float basePreIntensity = mix(0.7, 0.7, biomeHeight);
	float power = 2;
	float basePostIntensity = mix(3, 40, biomeHeight);
	float translate = mix(0, 50, biomeHeight);


	int heightOctaves = 11;
	float heightFreq = 0.1;
	float heightPers = mix(0.3, 0.4, biomeHeight);
	float heightPreIntensity = mix(0.8, 1.5, biomeHeight);


	float baseheight = noise(pos.xzy, baseOctaves, baseFreq, basePers) * basePreIntensity;
	float height = noise(pos.xzy, heightOctaves, heightFreq, heightPers) * heightPreIntensity;
	baseheight = pow(baseheight, power) * basePostIntensity;
	height = baseheight * height + translate;

	return height;
}

vec3 calculateNormal(vec3 p1) {
	float delta = 0.5f;

	vec3 p2 = (p1 + vec3(delta, 0.0f, 0.0f)) * vec3(1.0f, 0.0f, 1.0f);
	vec3 p3 = (p1 + vec3(0.0f, 0.0f, -delta)) * vec3(1.0f, 0.0f, 1.0f);

	p2.y = getHeight(p2);
	p3.y = getHeight(p3);

	vec3 u = p2 - p1;
	vec3 v = p3 - p1;

	vec3 normal = vec3(0.0f);
	normal.x = (u.y * v.z) - (u.z * v.y);
	normal.y = (u.z * v.x) - (u.x * v.z);
	normal.z = (u.x * v.y) - (u.y * v.x);

	return normalize(normal);
}

vec3 calculateTan(vec3 norm) {
	vec3 tangent;
	vec3 binormal;

	vec3 c1 = cross(norm, vec3(0.0f, 0.0f, 1.0f));
	vec3 c2 = cross(norm, vec3(0.0f, 1.0f, 0.0f));

	if (length(c1) > length(c2))
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	tangent = normalize(tangent);

	return tangent;
}

vec3 calculateBinorm(vec3 norm) {
	vec3 binormal = cross(norm, calculateTan(norm));
	binormal = normalize(binormal);
	return binormal;
}

void main() {
	vec4 pos = (gl_TessCoord.x * gl_in[0].gl_Position +
		gl_TessCoord.y * gl_in[1].gl_Position +
		gl_TessCoord.z * gl_in[2].gl_Position);

	vec2 Tex = (gl_TessCoord.x * TE_vertex_tex[0] +
		gl_TessCoord.y * TE_vertex_tex[1] +
		gl_TessCoord.z * TE_vertex_tex[2]);

	float height = getHeight(pos.xyz);

	pos.y = height;

	vertex_norm = calculateNormal(pos.xyz);
	vertex_tan = calculateTan(vertex_norm);
	vertex_bi = calculateBinorm(vertex_norm);
	vertex_pos = pos.xyz;

	vertex_tex = Tex;
	gl_Position = P * V * pos;

}