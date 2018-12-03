#version 450 core

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec3 out_framePos;

uniform vec3 u_frameSize;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

void main() {
    out_framePos = in_pos * vec3(0.5f, 0.5f, -0.5f) + 0.5f;

    gl_Position = u_projMat * u_viewMat * vec4(in_pos * u_frameSize * 0.5f, 1.0f);
}
