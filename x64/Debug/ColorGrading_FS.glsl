#version 430 core

in  vec2 v2f_TexCoords;
out vec4 Color_;

uniform sampler2D u_Texture2D;
uniform sampler3D u_Grading3D;
uniform vec2 lutSize;

vec3 colorGrade(mediump sampler3D lut, const vec3 x) 
{
    // Alexa LogC EI 1000
    const float a = 5.555556;
    const float b = 0.047996;
    const float c = 0.244161 / log2(10.0);
    const float d = 0.386036;
    vec3 logc = c * log2(a * x + b) + d;

    // Remap to sample pixel centers
    logc = lutSize.x + logc * lutSize.y;

    return textureLod(lut, logc, 0.0).rgb;
}

void main()
{
	vec3 TexelColor = texture(u_Texture2D, v2f_TexCoords).rgb;
	TexelColor = colorGrade(u_Grading3D, TexelColor);
	Color_ = vec4(TexelColor, 1.0f);
}