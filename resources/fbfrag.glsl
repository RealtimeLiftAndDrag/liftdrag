#version 330 core

in vec3 vertex_pos;
in vec2 vertex_tex;

out vec4 color;

uniform sampler2D tex;

void main() {
    vec4 tcol = texture(tex, vertex_tex);
    //tcol.rg=vertex_tex;
    color = tcol;
    color.a = 1.0f;
}
