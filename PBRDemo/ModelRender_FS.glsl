#version 430 core

#define PI 3.1415926
#define FLT_EPS            1e-5
#define saturateMediump(x) x
#define saturate(x)        clamp(x, 0.0, 1.0)
#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS            0.002025


in  vec3 v2f_FragPosInViewSpace;
in  vec2 v2f_TexCoords;
in  vec3 v2f_Normal;

out vec4 Albedo_;

uniform vec3 lightPos;
uniform vec3 lightDirection;
uniform vec4 lightColor;
uniform vec3 cameraPos;
uniform vec4 sun;
uniform float exposure;
uniform float iblLuminance;

uniform vec4  baseColor;
uniform float metallic;
uniform float roughness;
uniform float reflectance;
uniform vec4  emissive;
uniform float ambientOcclusion;
uniform vec3  material_sheenColor;
uniform float material_sheenRoughness;
uniform float material_clearCoat;
uniform float material_clearCoatRoughness;


uniform vec3 frame_iblSH[9];

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform samplerCube environmentCubeMap;

highp mat3  shading_tangentToWorld;   // TBN matrix
highp vec3  shading_position;         // position of the fragment in world space
      vec3  shading_view;             // normalized vector from the fragment to the eye
      vec3  shading_normal;           // normalized transformed normal, in world space
      vec3  shading_geometricNormal;  // normalized geometric normal, in world space
      vec3  shading_reflected;        // reflection of view about normal
      float shading_NoV;              // dot(normal, view), always strictly >= MIN_N_DOT_V


vec3 getWorldPosition()
{
    return v2f_FragPosInViewSpace;
}

vec3 getWorldCameraPosition()
{
    return cameraPos;
}

vec3 getWorldViewVector()
{
    return cameraPos;
}

vec3 getWorldNormalVector() 
{
    return shading_normal;
}

vec3 getWorldGeometricNormalVector() 
{
    return shading_geometricNormal;
}

vec3 getWorldReflectedVector() 
{
    return shading_reflected;
}

float getNdotV() 
{
    return shading_NoV;
}

float getExposure()
{
    return exposure;
}

void computeShadingParams() 
{
    highp vec3 n = v2f_Normal;
    shading_geometricNormal = normalize(n);
    shading_position = v2f_FragPosInViewSpace.xyz;
    shading_view = normalize(getWorldCameraPosition()-shading_position);
}

#define MIN_N_DOT_V 1e-4
float clampNoV(float NoV) 
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return max(NoV, MIN_N_DOT_V);
}

vec3 Irradiance_SphericalHarmonics(const vec3 n) 
{
    return max(
          frame_iblSH[0]
        + frame_iblSH[1] * (n.y)
        + frame_iblSH[2] * (n.z)
        + frame_iblSH[3] * (n.x)
        + frame_iblSH[4] * (n.y * n.x)
        + frame_iblSH[5] * (n.y * n.z)
        + frame_iblSH[6] * (3.0 * n.z * n.z - 1.0)
        + frame_iblSH[7] * (n.z * n.x)
        + frame_iblSH[8] * (n.x * n.x - n.y * n.y)
        , 0.0);
}

struct Light 
{
    vec4 colorIntensity;  //rgb和预曝光强度
    //位置
    highp vec3 worldPosition;
    //方向
    highp vec3 direction;
    
    
    vec3 l;
    float NoL;

    //衰减系数
    float attenuation;
};


//标准模型参数
struct MaterialInputs 
{
    vec4  baseColor;
    float metallic;
    float roughness;
    float reflectance;
    vec4  emissive;
    float ambientOcclusion;

    vec3 sheenColor;
    float sheenRoughness;

    float clearCoat;
    float clearCoatRoughness;


};

struct SSAOInterpolationCache 
{
    highp vec4 weights;
    highp vec2 uv;

};

float max3(const vec3 v) 
{
    return max(v.x, max(v.y, v.z));
}

vec3 computeDiffuseColor(const vec4 baseColor, float metallic) 
{
    return baseColor.rgb * (1.0 - metallic);
}

vec3 computeF0(const vec4 baseColor, float metallic, float reflectance) 
{
    return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

float computeDielectricF0(float reflectance) 
{
    return 0.16 * reflectance * reflectance;
}

float perceptualRoughnessToRoughness(float perceptualRoughness) 
{
    return perceptualRoughness * perceptualRoughness;
}


void prepareMaterial(const MaterialInputs material) 
{
    shading_normal = getWorldGeometricNormalVector();
    shading_NoV = clampNoV(dot(shading_normal, shading_view));
    shading_reflected = reflect(-shading_view, shading_normal);
}



float D_GGX(float roughness, float NoH, const vec3 h)
{
    float oneMinusNoHSquared = 1.0 - NoH * NoH;
    float a = NoH * roughness;
    float k = roughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return saturateMediump(d);
}

float pow5(float x) 
{
    float x2 = x * x;
    return x2 * x2 * x;
}

vec3 F_Schlick(const vec3 f0, float f90, float VoH) 
{
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

vec3 F_Schlick(const vec3 f0, float VoH) 
{
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

float F_Schlick(float f0, float f90, float VoH) 
{
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float V_SmithGGXCorrelated(float roughness, float NoV, float NoL) 
{
    float a2 = roughness * roughness;
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    float v = 0.5 / (lambdaV + lambdaL);
    return saturateMediump(v);
}


float distribution(float roughness, float NoH, const vec3 h) 
{
    return D_GGX(roughness, NoH, h);
}

float visibility(float roughness, float NoV, float NoL) 
{
    return V_SmithGGXCorrelated(roughness, NoV, NoL);
}

vec3 fresnel(const vec3 f0, float LoH) 
{
    float f90 = saturate(dot(f0, vec3(50.0 * 0.33)));
    return F_Schlick(f0, f90, LoH);
}

float Fd_Lambert() 
{
    return 1.0 / PI;
}

float V_Kelemen(float LoH) 
{
    // Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
    return saturateMediump(0.25 / (LoH * LoH));
}

float V_Neubelt(float NoV, float NoL) 
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return saturateMediump(1.0 / (4.0 * (NoL + NoV - NoL * NoV)));
}

float D_Charlie(float roughness, float NoH) 
{
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
    float invAlpha  = 1.0 / roughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}


float specularAO(float NoV, float visibility, float roughness, const in SSAOInterpolationCache cache) 
{
    float specularAO = 1.0;
    return specularAO;
}

float distributionClearCoat(float roughness, float NoH, const vec3 h) 
{
    return D_GGX(roughness, NoH, h);

}

float visibilityClearCoat(float LoH) 
{

    return V_Kelemen(LoH);

}

float distributionCloth(float roughness, float NoH) 
{

    return D_Charlie(roughness, NoH);

}

float visibilityCloth(float NoV, float NoL) 
{
    return V_Neubelt(NoV, NoL);
}

struct PixelParams 
{
    vec3  diffuseColor;
    float perceptualRoughness;
    float perceptualRoughnessUnclamped;
    vec3  f0;
    float roughness;
    vec3  dfg;
    vec3  energyCompensation;

    float clearCoat;
    float clearCoatPerceptualRoughness;
    float clearCoatRoughness;
    vec3  sheenColor;
    float sheenRoughness;
    float sheenPerceptualRoughness;
    float sheenScaling;
    float sheenDFG;

};

/*
vec3 anisotropicLobe(const PixelParams pixel, const Light light, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) {

    vec3 l = light.l;
    vec3 t = pixel.anisotropicT;
    vec3 b = pixel.anisotropicB;
    vec3 v = shading_view;

    float ToV = dot(t, v);
    float BoV = dot(b, v);
    float ToL = dot(t, l);
    float BoL = dot(b, l);
    float ToH = dot(t, h);
    float BoH = dot(b, h);

    // Anisotropic parameters: at and ab are the roughness along the tangent and bitangent
    // to simplify materials, we derive them from a single roughness parameter
    // Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
    float at = max(pixel.roughness * (1.0 + pixel.anisotropy), MIN_ROUGHNESS);
    float ab = max(pixel.roughness * (1.0 - pixel.anisotropy), MIN_ROUGHNESS);

    // specular anisotropic BRDF
    float D = distributionAnisotropic(at, ab, ToH, BoH, NoH);
    float V = visibilityAnisotropic(pixel.roughness, at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
    vec3  F = fresnel(pixel.f0, LoH);

    return (D * V) * F;
}
*/

vec3 isotropicLobe(const PixelParams pixel, const Light light, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) 
{

    float D = distribution(pixel.roughness, NoH, h);
    float V = visibility(pixel.roughness, NoV, NoL);
    vec3  F = fresnel(pixel.f0, LoH);
    return (D * V) * F;
}


float diffuse(float roughness, float NoV, float NoL, float LoH) 
{
    return Fd_Lambert();
}

vec3 specularLobe(const PixelParams pixel, const Light light, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) 
{
    return isotropicLobe(pixel, light, h, clamp(NoV,0,1), clamp(NoL, 0,1), clamp(NoH,0,1), clamp(LoH,0,1));
}

vec3 diffuseLobe(const PixelParams pixel, float NoV, float NoL, float LoH) 
{
    return pixel.diffuseColor * diffuse(pixel.roughness, NoV, NoL, LoH);
}

vec3 sheenLobe(const PixelParams pixel, float NoV, float NoL, float NoH) 
{
    float D = distributionCloth(pixel.sheenRoughness, NoH);
    float V = visibilityCloth(NoV, NoL);

    return (D * V) * pixel.sheenColor;
}

float clearCoatLobe(const PixelParams pixel, const vec3 h, float NoH, float LoH, out float Fcc) 
{
    float clearCoatNoH = NoH;

    // clear coat specular lobe
    float D = distributionClearCoat(pixel.clearCoatRoughness, clearCoatNoH, h);
    float V = visibilityClearCoat(LoH);
    float F = F_Schlick(0.04, 1.0, LoH) * pixel.clearCoat; // fix IOR to 1.5

    Fcc = F;
    return D * V * F;
}

vec3 surfaceShading(const PixelParams pixel, const Light light, float occlusion) 
{
    vec3 h = normalize(shading_view + light.l);
    float NoV = shading_NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading_normal, h));
    float LoH = saturate(dot(light.l, h));
    vec3 Fr = specularLobe(pixel, light, h, NoV, NoL, NoH, LoH);
    vec3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);
    vec3 color = Fd + Fr * pixel.energyCompensation;

    color *= pixel.sheenScaling;
    color += sheenLobe(pixel, NoV, NoL, NoH);

    float Fcc;
    float clearCoat = clearCoatLobe(pixel, h, NoH, LoH, Fcc);
    float attenuation = 1.0 - Fcc;

    color *= attenuation;
    color += clearCoat;

    return (color * light.colorIntensity.rgb)*(light.colorIntensity.w * NoL *light.attenuation);

}
void unpremultiply(inout vec4 color) 
{
    color.rgb /= max(color.a, FLT_EPS);
}

void getCommonPixelParams(const MaterialInputs material, inout PixelParams pixel) 
{
    vec4 baseColor = material.baseColor;
    unpremultiply(baseColor);
    float reflectance = computeDielectricF0(material.reflectance);
    pixel.f0 = computeF0(baseColor, material.metallic, reflectance);
    pixel.diffuseColor = computeDiffuseColor(baseColor, material.metallic);
}

void getRoughnessPixelParams(const MaterialInputs material, inout PixelParams pixel) 
{
    float perceptualRoughness = material.roughness;
    pixel.perceptualRoughnessUnclamped = perceptualRoughness;
    pixel.perceptualRoughness = clamp(perceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    pixel.roughness = perceptualRoughnessToRoughness(pixel.perceptualRoughness);
}

void getSheenPixelParams(const MaterialInputs material, inout PixelParams pixel) 
{
    pixel.sheenColor = material.sheenColor;
    float sheenPerceptualRoughness = material.sheenRoughness;
    sheenPerceptualRoughness = clamp(sheenPerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    pixel.sheenPerceptualRoughness = sheenPerceptualRoughness;
    pixel.sheenRoughness = perceptualRoughnessToRoughness(sheenPerceptualRoughness);
}

vec3 PrefilteredDFG_LUT(float lod, float NoV) 
{
    // coord = sqrt(linear_roughness), which is the mapping used by cmgen.
    return textureLod(brdfLUT, vec2(NoV, lod), 0.0).rgb;
}

//------------------------------------------------------------------------------
// IBL environment BRDF dispatch
//------------------------------------------------------------------------------

vec3 prefilteredDFG(float perceptualRoughness, float NoV) 
{
    // PrefilteredDFG_LUT() takes a LOD, which is sqrt(roughness) = perceptualRoughness
    return PrefilteredDFG_LUT(perceptualRoughness, NoV);
}

vec3 f0ClearCoatToSurface(const vec3 f0) 
{
    // Approximation of iorTof0(f0ToIor(f0), 1.5)
    // This assumes that the clear coat layer has an IOR of 1.5
    return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
}


void getClearCoatPixelParams(const MaterialInputs material, inout PixelParams pixel) 
{

    pixel.clearCoat = material.clearCoat;

    // Clamp the clear coat roughness to avoid divisions by 0
    float clearCoatPerceptualRoughness = material.clearCoatRoughness;
    clearCoatPerceptualRoughness =
            clamp(clearCoatPerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

    pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
    pixel.clearCoatRoughness = perceptualRoughnessToRoughness(clearCoatPerceptualRoughness);

    // The base layer's f0 is computed assuming an interface from air to an IOR
    // of 1.5, but the clear coat layer forms an interface from IOR 1.5 to IOR
    // 1.5. We recompute f0 by first computing its IOR, then reconverting to f0
    // by using the correct interface
    pixel.f0 = mix(pixel.f0, f0ClearCoatToSurface(pixel.f0), pixel.clearCoat);
}

void getEnergyCompensationPixelParams(inout PixelParams pixel) 
{
    pixel.dfg = prefilteredDFG(pixel.perceptualRoughness, shading_NoV);
    pixel.energyCompensation = vec3(1.0);
    pixel.sheenDFG = prefilteredDFG(pixel.sheenPerceptualRoughness, shading_NoV).z;
    pixel.sheenScaling = 1.0 - max3(pixel.sheenColor) * pixel.sheenDFG;
}

void getPixelParams(const MaterialInputs material, out PixelParams pixel) 
{
    getCommonPixelParams(material, pixel);
    getSheenPixelParams(material, pixel);
    getClearCoatPixelParams(material, pixel);
    getRoughnessPixelParams(material, pixel);
    getEnergyCompensationPixelParams(pixel);
}


vec3 sampleSunAreaLight(const vec3 lightDirection) 
{
    if (sun.w >= 0.0) 
    {
        float LoR = dot(lightDirection, shading_reflected);
        float d = sun.x;
        highp vec3 s = shading_reflected - LoR * lightDirection;
        return LoR < d ?
                normalize(lightDirection * d + normalize(s) * sun.y) : shading_reflected;
    }
    return lightDirection;
}

Light getDirectionalLight() 
{
    Light light;
    // note: lightColorIntensity.w is always premultiplied by the exposure
    light.colorIntensity = lightColor;
    light.l = sampleSunAreaLight(lightDirection);
    light.attenuation = 1.0;
    light.NoL = saturate(dot(shading_normal, light.l));
    return light;
}


void evaluateDirectionalLight(const MaterialInputs material,
        const PixelParams pixel, inout vec3 color) 
{
    Light light = getDirectionalLight(); 
    float visibility = 1.0;
    color.rgb += surfaceShading(pixel, light, visibility);
}

vec3 getSpecularDominantDirection(const vec3 n, const vec3 r, float roughness) 
{
    return mix(r, n, roughness * roughness);
}

vec3 getReflectedVector(const PixelParams pixel, const vec3 n) 
{

    vec3 r = shading_reflected;
    return getSpecularDominantDirection(n, r, pixel.roughness);
}

float perceptualRoughnessToLod(float perceptualRoughness) 
{
    // The mapping below is a quadratic fit for log2(perceptualRoughness)+iblRoughnessOneLevel when
    // iblRoughnessOneLevel is 4. We found empirically that this mapping works very well for
    // a 256 cubemap with 5 levels used. But also scales well for other iblRoughnessOneLevel values.
    return 4.0f * perceptualRoughness * (2.0 - perceptualRoughness);
}

vec3 decodeDataForIBL(const vec4 data) 
{
    return data.rgb;
}


vec3 prefilteredRadiance(const vec3 r, float perceptualRoughness) 
{
    float lod = perceptualRoughnessToLod(perceptualRoughness);
    return decodeDataForIBL(textureLod(prefilterMap, r, lod));
}


vec3 specularDFG(const PixelParams pixel) 
{
    return mix(pixel.dfg.xxx, pixel.dfg.yyy, pixel.f0);
}

float singleBounceAO(float visibility)
{
   return visibility;
}


void evaluateSheenIBL(const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout vec3 Fd, inout vec3 Fr) 
{
    // Albedo scaling of the base layer before we layer sheen on top
    Fd *= pixel.sheenScaling;
    Fr *= pixel.sheenScaling;
    vec3 reflectance = pixel.sheenDFG * pixel.sheenColor;
    reflectance *= specularAO(shading_NoV, diffuseAO, pixel.sheenRoughness, cache);
    Fr += reflectance * prefilteredRadiance(shading_reflected, pixel.sheenPerceptualRoughness);

}


void evaluateClearCoatIBL(const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout vec3 Fd, inout vec3 Fr) 
{

    if (pixel.clearCoat > 0.0) 
    {

        float clearCoatNoV = shading_NoV;
        vec3 clearCoatR = shading_reflected;
        // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
        float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * pixel.clearCoat;
        float attenuation = 1.0 - Fc;
        Fd *= attenuation;
        Fr *= attenuation;

        // TODO: Should we apply specularAO to the attenuation as well?
        float specularAO = specularAO(clearCoatNoV, diffuseAO, pixel.clearCoatRoughness, cache);
        Fr += prefilteredRadiance(clearCoatR, pixel.clearCoatPerceptualRoughness) * (specularAO * Fc);
    }

}

void evaluateIBL(const MaterialInputs material, const PixelParams pixel, inout vec3 color) 
{

    SSAOInterpolationCache interpolationCache;


    // specular layer
    vec3 Fr = vec3(0.0);
    const vec4 ssrFr = vec4(0.0);

    vec3 E = specularDFG(pixel);
    if (ssrFr.a < 1.0) 
    { 
        // prevent reading the IBL if possible
        vec3 r = getReflectedVector(pixel, shading_normal);
        Fr = E * prefilteredRadiance(r, pixel.perceptualRoughness);
    }


    // Ambient occlusion
    float ssao = 1.0f;
    float diffuseAO = min(material.ambientOcclusion, ssao);
    float specularAO = 1.0f;


    // diffuse layer
    //float diffuseBRDF = 1.0f; // Fd_Lambert() is baked in the SH below
    float diffuseBRDF = singleBounceAO(diffuseAO);
    vec3 diffuseNormal = shading_normal;
    //vec3 Fd = vec3(0,0,0);

    vec3 diffuseIrradiance = texture(irradianceMap, diffuseNormal).rgb;
    diffuseIrradiance = Irradiance_SphericalHarmonics(diffuseNormal);
    vec3 Fd = pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * diffuseBRDF;
    //Fd = diffuseIrradiance;

     // sheen layer
    evaluateSheenIBL(pixel, diffuseAO, interpolationCache, Fd, Fr);
    // clear coat layer
    evaluateClearCoatIBL(pixel, diffuseAO, interpolationCache, Fd, Fr);


    Fr *= iblLuminance;
    Fd *= iblLuminance;

    color.rgb += Fr + Fd;


}

float computeDiffuseAlpha(float a) 
{
    return a;
}

vec4 evaluateLights(const MaterialInputs material) 
{
    PixelParams pixel;
    getPixelParams(material, pixel);

    vec3 color = vec3(0.0);
    evaluateIBL(material, pixel, color);

    evaluateDirectionalLight(material, pixel, color);

    //点光或者聚光
    //evaluatePunctualLights(material, pixel, color);

    color *= material.baseColor.a;

    return vec4(color, computeDiffuseAlpha(material.baseColor.a));
}

void addEmissive(const MaterialInputs material, inout vec4 color) 
{

    highp vec4 emissive = material.emissive;
    highp float attenuation = mix(1.0, getExposure(), emissive.w);
    attenuation *= color.a;
    color.rgb += emissive.rgb * attenuation;

}

vec4 evaluateMaterial(const MaterialInputs material) 
{
    vec4 color = evaluateLights(material);
    addEmissive(material, color);
    return color;
}

void main()
{
    MaterialInputs inputs;
    inputs.baseColor = baseColor;
    inputs.metallic = metallic;
    inputs.roughness = roughness;
    inputs.reflectance = reflectance; 
    inputs.emissive = emissive;
    inputs.ambientOcclusion = ambientOcclusion;
    inputs.sheenColor = material_sheenColor;
    inputs.sheenRoughness = material_sheenRoughness;
    inputs.clearCoat = material_clearCoat;
    inputs.clearCoatRoughness = material_clearCoatRoughness;

    computeShadingParams();
    prepareMaterial(inputs);

    vec4 hdrColor = evaluateMaterial(inputs); 
    Albedo_ = hdrColor;

}