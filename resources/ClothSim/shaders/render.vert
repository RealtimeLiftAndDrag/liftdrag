#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_norm;
layout (location = 2) out float out_blah;

uniform mat4 u_modelMat;
uniform mat3 u_normalMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

int sp_to_ii(ivec2 sp) {
    int si = sp.y * 31 + sp.x;
    return si * 6;
}
    

void main() {
    out_pos = vec3(u_modelMat * vec4(in_pos, 1.0f));
    out_norm = u_normalMat * in_norm;
    ivec2 p = ivec2(gl_VertexID % 32, gl_VertexID / 32);
    int ii0 = sp_to_ii(p + ivec2(-1, -1)) + 3;
    int jj0 = sp_to_ii(p + ivec2( 0,  0)) + 0;
    int ii1 = sp_to_ii(p + ivec2( 0, -1)) + 4;
    int jj1 = sp_to_ii(p + ivec2(-1,  0)) + 1;
    int ii2 = sp_to_ii(p + ivec2(-1, -1)) + 2;
    int jj2 = sp_to_ii(p + ivec2( 0,  0)) + 5;
    out_blah = float(ii0 / 32 == jj0 / 32 || ii1 / 32 == jj1 / 32 || ii2 / 32 == jj2 / 32);



    gl_Position = u_projMat * u_viewMat * vec4(out_pos, 1.0f);
}
