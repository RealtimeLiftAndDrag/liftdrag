#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_norm;

uniform mat4 u_modelMat;
uniform mat3 u_normalMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

void main() {
    out_pos = vec3(u_modelMat * vec4(in_pos, 1.0f));
    out_norm = u_normalMat * in_norm;

    gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(in_pos, 1.0f);
}
