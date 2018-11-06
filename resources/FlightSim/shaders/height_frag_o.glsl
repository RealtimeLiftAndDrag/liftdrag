#version 330 core
out vec4 color;
in vec2 frag_tex;
uniform sampler2D tex;
uniform sampler2D tex2;
void main()
{
	vec4 texcolor = texture(tex2, frag_tex * 100);
	color.rgb = texcolor.rgb;
	color.a = 1;
}
