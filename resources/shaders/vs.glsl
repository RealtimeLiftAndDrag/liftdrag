#version 410 core

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNor;
layout (location = 2) in vec2 vertTex;

out vec3 frag_pos;
out vec3 frag_normal;
out vec2 frag_tex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 MR;

void main() {
    frag_normal = vec4(MR * vec4(vertNor, 1.0f)).xyz;
    //vertex_normal = vertNor;
    vec4 pos = V * M * (vec4(vertPos, 1.0f));
    frag_pos = pos.xyz;
    gl_Position = P * pos;
    frag_tex = vertTex;	
}
