#version 450 core

layout (location = 0) in vec2 in_texCoord;

layout (location = 0) out vec4 out_color;

uniform sampler2D u_tex;

float isWithin(vec2 p, vec2 bottomLeft, vec2 topRight) {
    vec2 s = step(bottomLeft, p) - step(topRight, p);
    return s.x * s.y;   
}

void main() {
    float inside = isWithin(in_texCoord, vec2(0.0f), vec2(1.0f));

    vec3 inColor = texture(u_tex, in_texCoord).rgb;

    if (true) {
        if (inColor.b > 0.5f) inColor = vec3(0.0f, (inColor.b - 0.5f) * 2.0f, 0.0f);
        else if (inColor.b > 0.0f) inColor = vec3(inColor.b * 2.0f, 0.0f, 0.0f);
        else inColor = vec3(0.25f * max(inColor.r, inColor.g));
    }
    else {
        inColor.b = 0.0f;
    }

    ivec2 grid = (ivec2(gl_FragCoord.xy) & 0xF) >> 3;
    vec3 outColor = vec3(mix(0.1f, 0.2f, float(grid.x ^ grid.y)));

    out_color.rgb = mix(outColor, inColor, inside);
    out_color.a = 1.0f;
}
