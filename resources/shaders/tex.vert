#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec2 out_texCoord;

uniform vec4 u_viewBounds; // xy is position, zw is size, in texture space

void main() {
    gl_Position = vec4(in_pos * 2.0f - 1.0f, 0.0f, 1.0f);
    out_texCoord = u_viewBounds.xy + in_pos * u_viewBounds.zw;
}
