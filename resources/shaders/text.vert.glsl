#version 450 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in uint in_char;

out vec2 v2f_texCoords;

uniform vec2 u_fontSize;
uniform vec2 u_viewSize;
uniform vec2 u_linePos;

void main() {
    vec2 cell = vec2(in_char & 0xF, in_char >> 4);
    cell.x += in_pos.x; cell.y += 1.0f - in_pos.y;
    v2f_texCoords = cell / 16.0f;

    vec2 pos = in_pos;
    pos.x += gl_InstanceID;
    pos *= u_fontSize;
    pos += u_linePos;
    pos /= u_viewSize;
    gl_Position = vec4(pos * 2.0f - 1.0f, 0.0f, 1.0f);
}