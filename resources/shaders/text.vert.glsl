#version 450 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in uint in_char;

out vec2 v2f_texCoords;

uniform vec2 u_fontSize;
uniform vec2 u_viewSize;
uniform vec2 u_linePos;

void main() {
    v2f_texCoords = (vec2(in_char & 0xF, in_char >> 4) + vec2(in_pos.x, 1.0f - in_pos.y)) / 16.0f;

    gl_Position = vec4((u_linePos + vec2(in_pos.x + gl_InstanceID, in_pos.y) * u_fontSize) / u_viewSize * 2.0f - 1.0f, 0.0f, 1.0f);
}