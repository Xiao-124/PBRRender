#version 430 core

#define FLT_EPS            1e-5

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

vec4 resolveFragment(const highp vec2 uv) 
{
    vec4 color = textureLod(u_Texture2D, uv, 0.0);
    color.rgb *= 1.0 / (color.a + FLT_EPS);
    return color;
}


vec3 bloom(highp vec2 uv, const vec3 color) 
{
    vec3 result = vec3(0.0);

    //if (materialParams_bloom.x > 0.0) 
    //{
    //    vec3 bloom = textureLod(materialParams_bloomBuffer, uv, 0.0).rgb;
    //    result += bloom * materialParams.bloom.x;
    //}
    //
    //if (materialParams_bloom.w > 0.0) 
    //{
    //    float starburstMask = starburst(uv);
    //    vec3 flare = textureLod(materialParams_flareBuffer, uv, 0.0).rgb;
    //    result += flare * (materialParams.bloom.w * starburstMask);
    //}
    //
    //if (materialParams_bloom.z > 0.0) 
    //{
    //    float dirtIntensity = materialParams.bloom.z;
    //    vec3 dirt = textureLod(materialParams_dirtBuffer, uv, 0.0).rgb;
    //    result *= dirt * dirtIntensity;
    //}
    //result += color * materialParams_bloom.y;
    return result;
}


void main()
{
	vec3 TexelColor = texture(u_Texture2D, v2f_TexCoords).rgb;
	TexelColor = colorGrade(u_Grading3D, TexelColor);
	Color_ = vec4(TexelColor, 1.0f);

    //vec4 color = resolveFragment(v2f_TexCoords);
    //// Bloom
    //if (materialParams_bloom.x > 0.0) 
    //{
    //    color.rgb = bloom(uv, color.rgb);
    //}



}