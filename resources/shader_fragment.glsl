#version 410 core

in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
out vec4 color;

uniform vec3 campos;
uniform sampler2D tex;
uniform sampler2D tex2;

void main() {
    vec3 n = normalize(vertex_normal);
    vec3 lp = vec3(10.0f, -20.0f, -100.0f);
    vec3 ld = normalize(vertex_pos - lp);
    float diffuse = dot(n, ld);

    vec4 tcol = texture(tex, vertex_tex);
    color = tcol;
    color.a = (color.r + color.g + color.b) / 3.0f;
}
