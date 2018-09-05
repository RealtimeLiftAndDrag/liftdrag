#version 450 core

layout (location = 0) in float in_pos;

uniform vec2 u_origin;
uniform vec2 u_orient;
uniform vec2 u_delta;

void main() {
    vec2 p = u_origin + u_delta * float(gl_InstanceID);
    gl_Position = vec4(p + u_orient * in_pos, 0.0f, 1.0f);
}