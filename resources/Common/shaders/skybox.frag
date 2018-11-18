#version 450 core

layout (location = 0) in vec3 in_texCoord;

layout (location = 0) out vec4 out_color;

uniform samplerCube u_cubemap;

void main() {
    out_color.rgb = texture(u_cubemap, in_texCoord).rgb;
    out_color.a = 1.0f;
}
