#version 450 core

layout (location = 0) in vec2 in_vertPos;
layout (location = 1) in vec2 in_charPos;
layout (location = 2) in vec2 in_charTexCoords;

out vec2 v2f_texCoords;

uniform vec2 u_fontSize;
uniform vec2 u_viewportSize;

void main() {
    v2f_texCoords = in_charTexCoords + vec2(in_vertPos.x, 1.0f - in_vertPos.y) * (1.0f / 16.0f);

    vec2 pos = in_charPos + in_vertPos * u_fontSize;
    pos /= u_viewportSize;
    gl_Position = vec4(pos * 2.0f - 1.0f, 0.0f, 1.0f);
}
