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

    ivec2 grid = (ivec2(gl_FragCoord.xy) & 0xF) >> 3;
    vec3 outColor = vec3(mix(0.1f, 0.2f, float(grid.x ^ grid.y)));

    out_color.rgb = mix(outColor, inColor, inside);
    out_color.a = 1.0f;
}
