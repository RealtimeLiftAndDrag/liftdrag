#version 450 core

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;

// Constants -------------------------------------------------------------------

//const float k_pi = 3.14159265f;
const float k_ambience = 0.2f;
const float k_shininess = 8.0f;
const vec3 k_lightDir = normalize(vec3(1.0f, 1.0f, 1.0f));
const vec3 k_color = vec3(0.25f, 0.25f, 1.0f);

// Uniforms --------------------------------------------------------------------

uniform vec3 u_camPos; // camera position in world space

// Functions -------------------------------------------------------------------

/*vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0f - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hue2rgb(float h) {
    vec3 p = abs(fract(h + vec3(1.0f, 2.0f / 3.0f, 1.0f / 3.0f)) * 6.0f - 3.0f);
    return clamp(p - 1.0f, 0.0, 1.0);
}

vec3 hue2rgb_flat(float h) {
    vec3 rgb = hue2rgb(h);
    vec3 temp = rgb * vec3(0.546808925f, 0.766159252f, 0.337638860f);
    float pb = sqrt(dot(temp, temp));
    return rgb * (0.337638860f / pb);
}

// l: luminance [0, 1]
// uv: point in color space [-1, 1]
vec3 luv2rgb(vec3 luv) {
    const mat3 k_m = mat3( // XYZ -> sRGB conversion matrix
         3.2404542f, -0.9692660f,  0.0556434f,
        -1.5371385f,  1.8760108f, -0.2040259f,
        -0.4985314f,  0.0415560f,  1.0572252f
     );
    const vec2 k_uv0 = vec2(0.197833037f, 0.468330474f); // coords for D65 white point

    vec2 uv = k_uv0 + luv.yz / (13.0f * luv.x);
    vec3 xyz;
    if (luv.x > 0.08f) {
        xyz.y = 0.862068966f * luv.x + 0.137931034f;
        xyz.y *= xyz.y * xyz.y;
    }
    else {
        xyz.y = 0.110705646f * luv.x;
    }
    xyz.x = 2.25f * xyz.y * uv.x / uv.y;
    xyz.z = (3.0f / uv.y - 5.0f) * xyz.y - (1.0f / 3.0f) * xyz.x;
    return k_m * xyz;
}

// l: luminance [0, 1]
// c: chroma (similar to saturation) [0, 1]
// h: hue [0, 1]
vec3 lch2luv(vec3 lch) {
    lch.z *= 2.0f * k_pi;
    return luv2rgb(vec3(lch.x, lch.y * cos(lch.z), lch.y * sin(lch.z)));
}*/

vec3 safeNormalize(vec3 v) {
    float d = dot(v, v);
    return d > 0.0f ? v / sqrt(d) : vec3(0.0f);
}

void main() {
    vec3 viewDir = normalize(u_camPos - in_pos);
    vec3 norm = safeNormalize(in_norm);
    //if (!gl_FrontFacing) norm = -norm;

    // Phong
    if (true) {
        // Diffuse calculation
        float diffuse = max(dot(k_lightDir, norm), 0) + k_ambience; // Clamp to prevent color from reaching back side
        //float diffuseFactor = (dot(u_lightDir, norm) + 1) / 2; // Normalize so the color bleeds onto the back side

        // Specular calculation
        vec3 reflectDir = reflect(-k_lightDir, norm);
        float specular = pow(max(dot(viewDir, reflectDir), 0), k_shininess);

        //output color
        out_color.rgb = diffuse * k_color + vec3(specular * 0.5f);
        out_color.a = 1.0f;
    }

    // Rainbow
    else if (false) {
        //out_color.rgb = lch2luv(vec3(0.67f, 1.0f, vec3(gl_PrimitiveID) / float(u_primitiveCount - 1))) * diffuse;
        //out_color.rgb = lch2luv(vec3(0.67f, 1.0f, (gl_PrimitiveID % 16) / 16.0f));
        out_color.a = 1.0f;
    }

    // Normals
    else if (false) {
        out_color.rgb = norm * 0.5f + 0.5f;
        out_color.a = 1.0f;
    }


}
