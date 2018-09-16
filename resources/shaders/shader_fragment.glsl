#version 330 core
out vec4 color;

in vec2 frag_tex;
void main()
{
	color = vec4(1, 1, 1, 1);
	color.rg = frag_tex;
}
