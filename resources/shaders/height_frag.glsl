#version 330 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_norm;
in vec3 vertex_tan;
in vec3 vertex_bi;
in float colorO;
in float temperature;
in float humidity;

uniform sampler2D grassSampler;
uniform sampler2D snowSampler;
uniform sampler2D snowNormal;
uniform sampler2D sandSampler;
uniform sampler2D cliffSampler;
uniform sampler2D cliffNormal;
uniform sampler2D grassNormal;
uniform sampler2D sandNormal;
uniform vec3 camoff;
uniform vec3 campos;
uniform int meshsize;
uniform float resolution;

uniform float time;
uniform int drawGrey;

#define TERRAIN_TRANSFORM_RATE 400

vec3 blendRGB(vec3 rgbA, vec3 rgbB, vec3 rgbC, float alpha) {
	return (rgbA * alpha) + (rgbB * alpha) + (rgbC * alpha);
}

vec3 blendRGB(vec3 rgbA, vec3 rgbB, float alpha) {
	return (rgbA * alpha) + (rgbB * alpha);
}

vec3 blendLinear(vec3 rgbA, vec3 rgbB, float pos, float uBound, float lBound) {
	float alpha = (pos - lBound) / (uBound - lBound);
	return (rgbA * alpha) + (rgbB * (1 - alpha));
}

vec3 blendLerp(vec3 rgbA, vec3 rgbB, float alpha) {
	return (rgbA * alpha) + (rgbB * (1 - alpha));
}


vec3 getTextureDiffuse(vec3 normalMap, vec3 tex) {
	float sTime = sin(time);
	float cTime = cos(time);

	vec3 cliffNormalMap = normalMap;
	cliffNormalMap = (cliffNormalMap - vec3(0.5, 0.5, 0.5)) * 2;
	vec3 bumpNormal = (cliffNormalMap.x * vertex_tan) + (cliffNormalMap.y * vertex_bi) + (cliffNormalMap.z * vertex_norm);	//rotate normal into tangent space
	bumpNormal = normalize(bumpNormal);

	// Diffuse lighting
	vec3 distance = vec3(0, cTime * 1.1, sTime * 1.1);
	float diffuse = dot(bumpNormal, distance);
	return tex * max(0.1, diffuse * 1.0);
}

float normalizeFloat(float x, float min, float max) {
	return (x - min) / (max - min);
}

vec3 applyFog(vec3  rgb, float distance, float density) {
	float heightMod = pow(normalizeFloat(vertex_pos.y, 0.0f, 50.0f), 3.0f);
	float fogAmount = 1.0 - exp((-distance) * density * heightMod);
	fogAmount *= 0.5;
	vec3  fogColor = vec3(0.5, 0.6, 0.7);
	return mix(rgb, fogColor, fogAmount);
}


float oscillate(float t, float min, float max) {
	float halfRange = (max - min) / 2.0f;
	return (min + halfRange) + (cos(t / TERRAIN_TRANSFORM_RATE) * halfRange);
}

void main()
{
	float dist = distance(vertex_pos, vec3(0));
	vec2 texcoords = vertex_tex;
	float upperCutoff, upperBlend, midCutoff, midBlend, lowerCutoff;
	upperCutoff = 15 / colorO;
	upperBlend = 10 / colorO;
	midCutoff = 4 / colorO;
	midBlend = 3 / colorO;

	// Blend grass texture layers
	vec4 grassTexture = vec4(texture(grassSampler, texcoords * 10, 0).rgb * 1, 1.0);
	vec4 grassLower = vec4(texture(grassSampler, texcoords * 20 + 20, 0).rgb * 1, 1.0);
	vec4 grassUpper = vec4(texture(grassSampler, texcoords * 30 + 30, 0).rgb * 1, 1.0);
	vec4 grassDetiled = vec4(blendRGB(grassTexture.rgb, grassLower.rgb, grassUpper.rgb, 0.33), 1.0);

	vec4 grassNormalUpper = vec4(texture(snowNormal, texcoords * 10).rgb * 1.0, 1.0);
	vec4 grassNormalMid = vec4(texture(snowNormal, texcoords * 20).rgb * 1.0, 1.0);
	vec4 grassNormalDetiled = vec4(blendRGB(grassNormalUpper.rgb, grassNormalMid.rgb, 0.5), 1.0);

	vec3 grassDiffused = getTextureDiffuse(grassNormalDetiled.rgb, grassDetiled.rgb);
	//grassDiffused = grassDetiled.rgb;

	// Blend snow texture layers
	vec4 snowUpper = vec4(texture(snowSampler, texcoords * 20).rgb * 1.0, 1.0);
	vec4 snowMid = vec4(texture(snowSampler, texcoords * 30 + 30).rgb * 1.0, 1.0);
	vec4 snowLower = vec4(texture(snowSampler, texcoords * 40 + 40).rgb * 1.0, 1.0);
	vec4 snowDetiled = vec4(blendRGB(snowUpper.rgb, snowMid.rgb, snowLower.rgb, 0.333), 1.0);

	vec4 snowNormalUpper = vec4(texture(snowNormal, texcoords * 20).rgb * 1.0, 1.0);
	vec4 snowNormalLower = vec4(texture(snowNormal, texcoords * 30 + 50).rgb * 1.0, 1.0);
	vec4 snowNormalDetiled = vec4(blendRGB(snowNormalUpper.rgb, snowNormalLower.rgb, 0.5), 1.0);

	vec3 snowDiffused = getTextureDiffuse(snowNormalDetiled.rgb, snowDetiled.rgb);

	vec4 sandTexture = texture(sandSampler, texcoords * 20);

	vec4 sandNormalDetiled = vec4(texture(sandNormal, texcoords * 20).rgb * 1.0, 1.0);
	vec3 sandDiffused = getTextureDiffuse(sandNormalDetiled.rgb, sandTexture.rgb);
	//sandDiffused = sandTexture.rgb;

	vec4 cliffTextureLower = vec4(texture(cliffSampler, texcoords * 10).rgb * 0.7, 1.0);
	vec4 cliffTextureMid = vec4(texture(cliffSampler, texcoords * 20).rgb * 0.7, 1.0);
	vec4 cliffTextureUpper = vec4(texture(cliffSampler, texcoords * 30).rgb * 0.7, 1.0);
	vec4 cliffDetiled = vec4(blendRGB(cliffTextureLower.rgb, cliffTextureMid.rgb, cliffTextureUpper.rgb, 0.333), 1.0);

	vec4 cliffNormalTexUpper = vec4(texture(cliffNormal, texcoords * 5).rgb, 1.0);
	vec4 cliffNormalTexLower = vec4(texture(cliffNormal, texcoords * 10).rgb, 1.0);
	vec4 cliffNormalDetiled = vec4(blendRGB(cliffNormalTexUpper.rgb, cliffNormalTexLower.rgb, 0.5), 1.0);

	vec3 cliffDiffused = getTextureDiffuse(cliffNormalDetiled.rgb, cliffDetiled.rgb);

	color.rgb = mix(mix(grassDiffused.rgb, sandDiffused.rgb, humidity), snowDiffused.rgb, temperature);

	float y = vertex_pos.y;


	// Blend cliff texture using the y component of the normal
	vec3 cliffMixed = mix(cliffDiffused, color.rgb, pow(vertex_norm.y, 5));
	// Reduce cliffs at lower vertex Y values
	color.rgb = mix(color.rgb, cliffMixed, min(vertex_pos.y / 0.7f, 1));


	float sTime = sin(time);
	float cTime = cos(time);

	if (drawGrey == 1) {
		color.rgb = vec3(0.6);
	}
	else {

		color.rgb = applyFog(color.rgb, length(campos.xz - vertex_pos.xz), 0.01f);
		// Diffuse lighting
		vec3 lightSource = vec3(0.0f, cTime, cTime) * 1000;
		vec3 distance = normalize(campos - lightSource);
		float diffuse = dot(vertex_norm, distance);
		color.rgb *= max(0.5, diffuse * 0.6);



		// Specular lighting
		vec3 cd = normalize(vertex_pos - campos);
		vec3 h = normalize(cd + distance);
		float spec = dot(vertex_norm, h);
		spec = clamp(spec, 0, 1);
		spec = pow(spec, 5);

	}
	//color.rgb = vec3(temperature);
	//color.rgb = vec3(colorO);

	color.a = 1;

	
}

