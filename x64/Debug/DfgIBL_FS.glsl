#version 430 core
out vec3 FragColor;
in vec2 TexCoords;

const float PI = 3.14159265359;
#define saturate(x)  clamp(x, 0, 1)


vec2 hammersley(uint i, float iN) 
{
    float tof = 0.5f / 0x80000000U;
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return vec2(i * iN, bits * tof );
}


float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
//vec2 Hammersley(uint i, uint N)
//{
//	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
//}

vec3 hemisphereImportanceSampleDggx(vec2 u, float a) 
{ 
    // pdf = D(a) * cosTheta
    const float phi = 2.0f * PI * u.x;
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    const float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    const float cosTheta = sqrt(cosTheta2);
    const float sinTheta = sqrt(1 - cosTheta2);
    return vec3( sinTheta * cos(phi), sinTheta * sin(phi), cosTheta );
}

float Visibility(float NoV, float NoL, float a) 
{
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // Height-correlated GGX
    const float a2 = a * a;
    const float GGXL = NoV * sqrt((NoL - NoL * a2) * NoL + a2);
    const float GGXV = NoL * sqrt((NoV - NoV * a2) * NoV + a2);
    return 0.5f / (GGXV + GGXL);
}

float pow5(float x) 
{
    const float x2 = x * x;
    return x2 * x2 * x;
}

vec2 DFV_Multiscatter(float NoV, float linearRoughness, int numSamples) 
{
    vec2 r = vec2(0, 0);
    const vec3 V = vec3(sqrt(1 - NoV * NoV), 0, NoV);
    for (int i = 0; i < numSamples; i++) 
    {
        const vec2 u = hammersley(i, 1.0f / numSamples);
        const vec3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const vec3 L = 2 * dot(V, H) * H - V;
        const float VoH = saturate(dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) 
        {
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow5(1 - VoH);
            r.x += v * Fc;
            r.y += v;
        }
    }
    return r * (4.0f / numSamples);
}

vec3  hemisphereUniformSample(vec2 u) 
{ 
    // pdf = 1.0 / (2.0 * PI);
    const float phi = 2.0f * PI * u.x;
    const float cosTheta = 1 - u.y;
    const float sinTheta = sqrt(1 - cosTheta * cosTheta);
    return vec3( sinTheta * cos(phi), sinTheta * sin(phi), cosTheta );
}

float VisibilityAshikhmin(float NoV, float NoL, float a) 
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return 1 / (4 * (NoL + NoV - NoL * NoV));
}


float DistributionCharlie(float NoH, float linearRoughness) 
{
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
    float a = linearRoughness;
    float invAlpha = 1 / a;
    float cos2h = NoH * NoH;
    float sin2h = 1 - cos2h;
    return (2.0f + invAlpha) * pow(sin2h, invAlpha * 0.5f) / (2.0f *  PI);
}

float DFV_Charlie_Uniform(float NoV, float linearRoughness, int numSamples) 
{
    float r = 0.0;
    const vec3 V = vec3(sqrt(1 - NoV * NoV), 0, NoV);
    for (int i = 0; i < numSamples; i++) 
    {
        const vec2 u = hammersley(i, 1.0f / numSamples);
        const vec3 H = hemisphereUniformSample(u);
        const vec3 L = 2 * dot(V, H) * H - V;
        const float VoH = saturate(dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) 
        {
            const float v = VisibilityAshikhmin(NoV, NoL, linearRoughness);
            const float d = DistributionCharlie(NoH, linearRoughness);
            r += v * d * NoL * VoH; // VoH comes from the Jacobian, 1/(4*VoH)
        }
    }
    // uniform sampling, the PDF is 1/2pi, 4 comes from the Jacobian
    return r * (4.0f * 2.0f *  PI / numSamples);
}


void main()
{

    float NoV = TexCoords.x;
    float linear_roughness = TexCoords.y * TexCoords.y;

    vec3 r = vec3( DFV_Multiscatter(NoV, linear_roughness, 1024), 0 );
    r.b = float(DFV_Charlie_Uniform(NoV, linear_roughness, 4096));
    FragColor = r;




}