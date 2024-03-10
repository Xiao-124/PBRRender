#version 430 core


#define MIN_ROUGHNESS            0.002025
#define PI 3.1415926
#define saturate(x)        clamp(x, 0.0, 1.0)

// we default to highp in this shader
precision highp float;
precision highp int;

out vec4 FragColor;
in vec2 TexCoords;
uniform float input_roughness[16];

float log4(const float x) 
{
    return log2(x) * 0.5;
}

vec2 hammersley(const uint index, const float sampleCount) 
{
    float invNumSamples = 1.0 / sampleCount;
    const float tof = 0.5 / float(0x80000000U);
    uint bits = index;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return vec2(float(index) * invNumSamples, float(bits) * tof);
}

float DistributionGGX(const float NoH, const float a) 
{
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    float f = (a - 1.0) * ((a + 1.0) * (NoH * NoH)) + 1.0;
    return (a * a) / (PI * f * f);
}

vec3 hemisphereImportanceSampleDggx(const vec2 u, const float a) 
{ 
    // pdf = D(a) * cosTheta
    float phi = 2.0 * PI * u.x;
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    float cosTheta2 = (1.0 - u.y) / (1.0 + (a + 1.0) * ((a - 1.0) * u.y));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

vec3 hemisphereCosSample(vec2 u) 
{ 
    // pdf = cosTheta / PI;
    float phi = 2.0 * PI * u.x;
    float cosTheta2 = 1.0 - u.y;
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

vec4 computeWeight(const uint index, const float roughness) 
{
    float sampleCount = 1024;
    vec2 u = hammersley(index, sampleCount);
    vec3 H = hemisphereImportanceSampleDggx(u, roughness);
    float NoH = H.z;
    float NoH2 = H.z * H.z;
    float NoL = saturate(2.0 * NoH2 - 1.0);
    vec3 L = vec3(2.0 * NoH * H.x, 2.0 * NoH * H.y, NoL);
    float pdf = DistributionGGX(NoH, max(MIN_ROUGHNESS, roughness)) * 0.25;
    float invOmegaS = sampleCount * pdf;
    float l = -log4(invOmegaS);
    return vec4(L, l);
}

vec4 computeWeightIrradiance(const uint index) 
{
    float sampleCount = 1024;
    vec2 u = hammersley(index, sampleCount);
    vec3 L = hemisphereCosSample(u);
    float pdf = L.z / PI;
    float invOmegaS = sampleCount * pdf;
    float l = -log4(invOmegaS);
    return vec4(L, l);
}



void main() 
{ 
    vec2 uv = vec2(TexCoords.x, TexCoords.y); // interpolated at pixel's center
    uv *= vec2(5.0f, 1024.0f);

    uint lod = uint(uv.x);
    uint index = uint(uv.y);
    float roughness = input_roughness[lod];
    vec4 weigets = computeWeight(index, roughness);
    FragColor = vec4(weigets.x, weigets.y, weigets.z, weigets.w);
    //FragColor = vec4(1.0f,0.0f,0.0f,1.0f);
    //if (materialConstants_irradiance) 
    //{
    //    postProcess.weight = computeWeightIrradiance(index);
    //} 
    //else 
    //{
    //    
    //}
}

