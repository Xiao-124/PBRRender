#version 330 core

out vec4 color;
uniform vec3 lightColor;
uniform sampler2D u_LightSourceTexture;
in vec2 texcoord;
void main()
{
	color = vec4(lightColor*texture(u_LightSourceTexture, texcoord).rgb, 1.0f);
}