#version 450 core

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_norm;
layout (location = 2) out vec2 out_texCoord;

// Uniforms --------------------------------------------------------------------

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;
uniform mat3 u_normMat;

// Functions -------------------------------------------------------------------

void main() {
    out_pos = (u_viewMat * u_modelMat * (vec4(in_pos, 1.0f))).xyz;
    out_norm = u_normMat * in_norm;
    out_texCoord = in_texCoord;	

    gl_Position = u_projMat * vec4(out_pos, 1.0f);
}
