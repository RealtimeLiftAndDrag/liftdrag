#version 450 core

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec3 out_texCoord;

uniform mat4 u_viewMat;
uniform mat4 u_projMat;

void main() {
    out_texCoord = in_pos;

    gl_Position = u_projMat * vec4((u_viewMat * vec4(in_pos, 0.0f)).xyz, 1.0f);
    gl_Position.z = gl_Position.w;
}
