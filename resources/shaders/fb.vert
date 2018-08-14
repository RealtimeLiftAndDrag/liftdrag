#version 450 core

layout (location = 0) in vec2 in_pos;

layout (location = 0) out vec2 out_texCoord;

void main() {
	gl_Position = vec4(in_pos, 0.0f, 1.0f);
	out_texCoord = in_pos * 0.5f + 0.5f;
}
