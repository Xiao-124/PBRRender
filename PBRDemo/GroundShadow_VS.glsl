#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;

out VS_OUT 
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

layout(std140, binding = 0) uniform u_Matrices4ProjectionWorld
{
	mat4 u_ProjectionMatrix;
	mat4 u_ViewMatrix;
};

uniform mat4 u_ModelMatrix;
uniform mat4 u_LightVPMatrix;

void main()
{
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * u_ModelMatrix * vec4(position, 1.0f);
    vs_out.FragPos = vec3(u_ModelMatrix * vec4(position, 1.0));
    vs_out.Normal = transpose(inverse(mat3(u_ModelMatrix))) * normal;
    vs_out.TexCoords = texCoords;
    vs_out.FragPosLightSpace = u_LightVPMatrix * vec4(vs_out.FragPos, 1.0);
}