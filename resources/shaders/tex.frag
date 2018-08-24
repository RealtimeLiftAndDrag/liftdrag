#version 450 core

layout (location = 0) in vec2 in_texCoord;

layout (location = 0) out vec4 out_color;

uniform sampler2D u_tex;

void main() {
    out_color.rgb = texture(u_tex, in_texCoord).rgb;
    out_color.a = 1.0f;
}
