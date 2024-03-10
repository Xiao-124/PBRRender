
#include "ColorGradingPass.h"
#include "ColorSpaceUtils.h"
#include "ToneMapper.h"
#include "Utils.h"
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <tuple>
#include "Common.h"
#include "Interface.h"
#include "Shader.h"
void CColorGradingPass::setQuality(CColorGradingPass::QualityLevel qualityLevel) noexcept
{
    switch (qualityLevel) {
    case CColorGradingPass::QualityLevel::LOW:
        format = LutFormat::INTEGER;
        dimension = 16;
        break;
    case CColorGradingPass::QualityLevel::MEDIUM:
        format = LutFormat::INTEGER;
        dimension = 32;
        break;
    case CColorGradingPass::QualityLevel::HIGH:
        format = LutFormat::FLOAT;
        dimension = 32;
        break;
    case CColorGradingPass::QualityLevel::ULTRA:
        format = LutFormat::FLOAT;
        dimension = 64;
        break;
    }
}

void CColorGradingPass::setFormat(LutFormat format) noexcept
{
    this->format = format;
    
}

void CColorGradingPass::setDimensions(uint8_t dim) noexcept
{
    this->dimension = glm::clamp(+dim, 16, 64);
    
}

void CColorGradingPass::setToneMapper(const ToneMapper* toneMapper) noexcept
{
    this->toneMapper = toneMapper;
}


void CColorGradingPass::setLuminanceScaling(bool luminanceScaling) noexcept
{
    this->luminanceScaling = luminanceScaling;
}

void CColorGradingPass::setGamutMapping(bool gamutMapping) noexcept
{
    this->gamutMapping = gamutMapping;
}

void CColorGradingPass::setExposure(float exposure) noexcept
{
    this->exposure = exposure;
}

void CColorGradingPass::setNightAdaptation(float adaptation) noexcept
{
    this->nightAdaptation = glm::saturate(adaptation);
}

void CColorGradingPass::setWhiteBalance(float temperature, float tint) noexcept
{
    this->whiteBalance = glm::vec2{
        glm::clamp(temperature, -1.0f, 1.0f),
        glm::clamp(tint, -1.0f, 1.0f)
    };
}

void CColorGradingPass::setChannelMixer(
    glm::vec3 outRed, glm::vec3 outGreen, glm::vec3 outBlue) noexcept 
{
    this->outRed = glm::clamp(outRed, -2.0f, 2.0f);
    this->outGreen = glm::clamp(outGreen, -2.0f, 2.0f);
    this->outBlue = glm::clamp(outBlue, -2.0f, 2.0f);
    
}

void CColorGradingPass::setShadowsMidtonesHighlights(
    glm::vec4 shadows, glm::vec4 midtones, glm::vec4 highlights, glm::vec4 ranges) noexcept 
{
    //this->shadows = glm::max(shadows.rgb + shadows.w, 0.0f);
    //this->midtones = glm::max(midtones.rgb + midtones.w, 0.0f);
    //this->highlights = glm::max(highlights.rgb + highlights.w, 0.0f);

    ranges.x = glm::saturate(ranges.x); // shadows
    ranges.w = glm::saturate(ranges.w); // highlights
    ranges.y = glm::clamp(ranges.y, ranges.x + 1e-5f, ranges.w - 1e-5f); // darks
    ranges.z = glm::clamp(ranges.z, ranges.x + 1e-5f, ranges.w - 1e-5f); // lights
    this->tonalRanges = ranges;

    
}

void CColorGradingPass::setSlopeOffsetPower(
    glm::vec3 slope, glm::vec3 offset, glm::vec3 power) noexcept 
{
    this->slope = glm::max(glm::vec3(1e-5f), slope);
    this->offset = offset;
    this->power = glm::max(glm::vec3(1e-5f), power);
    
}

void CColorGradingPass::setContrast(float contrast) noexcept
{
    this->contrast = glm::clamp(contrast, 0.0f, 2.0f);
    
}

void CColorGradingPass::setVibrance(float vibrance) noexcept
{
    this->vibrance = glm::clamp(vibrance, 0.0f, 2.0f);
    
}

void CColorGradingPass::setSaturation(float saturation) noexcept
{
    this->saturation = glm::clamp(saturation, 0.0f, 2.0f);
    
}

void CColorGradingPass::setCurves(
    glm::vec3 shadowGamma, glm::vec3 midPoint, glm::vec3 highlightScale) noexcept 
{
    this->shadowGamma = glm::max(glm::vec3(1e-5f), shadowGamma);
    this->midPoint = glm::max(glm::vec3(1e-5f), midPoint);
    this->highlightScale = highlightScale;
    
}

void CColorGradingPass::setOutputColorSpace(
    const ColorSpace& colorSpace) noexcept 
{
    this->outputColorSpace = colorSpace;
    
}



#pragma clang diagnostic pop

//------------------------------------------------------------------------------
// Exposure
//------------------------------------------------------------------------------


inline glm::vec3 adjustExposure(glm::vec3 v, float exposure) 
{
    return v * std::exp2(exposure);
}

//------------------------------------------------------------------------------
// Purkinje shift/scotopic vision
//------------------------------------------------------------------------------

glm::vec3 scotopicAdaptation(glm::vec3 v, float nightAdaptation) noexcept 
{

    constexpr glm::vec3 L{ 7.696847f, 18.424824f,  2.068096f };
    constexpr glm::vec3 M{ 2.431137f, 18.697937f,  3.012463f };
    constexpr glm::vec3 S{ 0.289117f,  1.401833f, 13.792292f };
    constexpr glm::vec3 R{ 0.466386f, 15.564362f, 10.059963f };

    
    glm::mat3 LMS_to_RGB = glm::inverse(glm::transpose(glm::mat3{ L, M, S }));

    // Maximal LMS cone sensitivity, Cao et al. Table 1
    constexpr glm::vec3 m{ 0.63721f, 0.39242f, 1.6064f };
    // Strength of rod input, free parameters in Cao et al., manually tuned for our needs
    // We follow Kirk & O'Brien who recommend constant values as opposed to Cao et al.
    // who propose to adapt those values based on retinal illuminance. We instead offer
    // artistic control at the end of the process
    // The vector below is {k1, k1, k2} in Kirk & O'Brien, but {k5, k5, k6} in Cao et al.
    constexpr glm::vec3 k{ 0.2f, 0.2f, 0.3f };

    // Transform from opponent space back to LMS
    glm::mat3 opponent_to_LMS
    {
        -0.5f, 0.5f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 1.0f
    };

    // The constants below follow Cao et al, using the KC pathway
    // Scaling constant
    constexpr float K_ = 45.0f;
    // Static saturation
    constexpr float S_ = 10.0f;
    // Surround strength of opponent signal
    constexpr float k3 = 0.6f;
    // Radio of responses for white light
    constexpr float rw = 0.139f;
    // Relative weight of L cones
    constexpr float p = 0.6189f;

    // Weighted cone response as described in Cao et al., section 3.3
    // The approximately linear relation defined in the paper is represented here
    // in matrix form to simplify the code
    glm::mat3 weightedRodResponse = (K_ / S_) * glm::mat3{
        -(k3 + rw),       p * k3,          p * S_,
        1.0f + k3 * rw, (1.0f - p) * k3, (1.0f - p) * S_,
        0.0f,            1.0f,            0.0f
    } *glm::mat3{ k,k,k } *glm::inverse(glm::mat3{ m,m,m });


    // Move to log-luminance, or the EV values as measured by a Minolta Spotmeter F.
    // The relationship is EV = log2(L * 100 / 14), or 2^EV = L / 0.14. We can therefore
    // multiply our input by 0.14 to obtain our log-luminance values.
    // We then follow Patry's recommendation to shift the log-luminance by ~ +11.4EV to
    // match luminance values to mesopic measurements as described in Rezagholizadeh &
    // Clark 2013,
    // The result is 0.14 * exp2(11.40) ~= 380.0 (we use +11.406 EV to get a round number)
    constexpr float logExposure = 380.0f;

    // Move to scaled log-luminance
    v *= logExposure;

    // Convert the scene color from Rec.709 to LMSR response
    glm::vec4 q{ dot(v, L), dot(v, M), dot(v, S), dot(v, R) };
    // Regulated signal through the selected pathway (KC in Cao et al.)
    glm::vec3 g = glm::inversesqrt(1.0f + glm::max(glm::vec3{ 0.0f }, glm::vec3((0.33f / m) * (glm::rgb(q) + k * q.w))));

    // Compute the incremental effect that rods have in opponent space
    glm::vec3 deltaOpponent = weightedRodResponse * g * q.w * nightAdaptation;
    // Photopic response in LMS space
    glm::vec3 qHat = glm::rgb(q) + opponent_to_LMS * deltaOpponent;

    // And finally, back to RGB
    return (LMS_to_RGB * qHat) / logExposure;
}

//------------------------------------------------------------------------------
// White balance
//------------------------------------------------------------------------------


glm::mat3 adaptationTransform(glm::vec2 whiteBalance) noexcept 
{
    // See Mathematica notebook in docs/math/White Balance.nb
    float k = whiteBalance.x; // temperature
    float t = whiteBalance.y; // tint

    float x = ILLUMINANT_D65_xyY[0] - k * (k < 0.0f ? 0.0214f : 0.066f);
    float y = chromaticityCoordinateIlluminantD(x) + t * 0.066f;

    glm::vec3 lms = XYZ_to_CIECAT16 * xyY_to_XYZ({ x, y, 1.0f });
    return LMS_CAT16_to_Rec2020 * glm::mat3{ ILLUMINANT_D65_LMS_CAT16 / lms, ILLUMINANT_D65_LMS_CAT16 / lms,ILLUMINANT_D65_LMS_CAT16 / lms } *Rec2020_to_LMS_CAT16;
}


inline glm::vec3 chromaticAdaptation(glm::vec3 v, glm::mat3 adaptationTransform) 
{
    return adaptationTransform * v;
}

//------------------------------------------------------------------------------
// General color grading
//------------------------------------------------------------------------------

using ColorTransform = glm::vec3(*)(glm::vec3);


inline constexpr glm::vec3 channelMixer(glm::vec3 v, glm::vec3 r, glm::vec3 g, glm::vec3 b) 
{
    return { dot(v, r), dot(v, g), dot(v, b) };
}


inline glm::vec3 tonalRanges(
        glm::vec3 v, glm::vec3 luminance,
        glm::vec3 shadows, glm::vec3 midtones, glm::vec3 highlights,
        glm::vec4 ranges
    ) 
{
    // See the Mathematica notebook at docs/math/Shadows Midtones Highlight.nb for
    // details on how the curves were designed. The default curve values are based
    // on the defaults from the "Log" color wheels in DaVinci Resolve.
    float y = dot(v, luminance);

    // Shadows curve
    float s = 1.0f - glm::smoothstep(ranges.x, ranges.y, y);
    // Highlights curve
    float h = glm::smoothstep(ranges.z, ranges.w,  y);
    // Mid-tones curves
    float m = 1.0f - s - h;

    return v * s * shadows + v * m * midtones + v * h * highlights;
}


inline glm::vec3 colorDecisionList(glm::vec3 v, glm::vec3 slope, glm::vec3 offset, glm::vec3 power) 
{
    // Apply the ASC CSL in log space, as defined in S-2016-001
    v = v * slope + offset;
    glm::vec3 pv = pow(v, power);
    return glm::vec3{
            v.r <= 0.0f ? v.r : pv.r,
            v.g <= 0.0f ? v.g : pv.g,
            v.b <= 0.0f ? v.b : pv.b
    };
}


inline glm::vec3 contrast(glm::vec3 v, float contrast) 
{
    // Matches contrast as applied in DaVinci Resolve
    return MIDDLE_GRAY_ACEScct + contrast * (v - MIDDLE_GRAY_ACEScct);
}


inline glm::vec3 saturation(glm::vec3 v, glm::vec3 luminance, float saturation) 
{
    const glm::vec3 y = glm::vec3(glm::dot(v, luminance));
    return y + saturation * (v - y);
}


inline glm::vec3 vibrance(glm::vec3 v, glm::vec3 luminance, float vibrance) 
{
    float r = v.r - glm::max(v.g, v.b);
    float s = (vibrance - 1.0f) / (1.0f + std::exp(-r * 3.0f)) + 1.0f;
    glm::vec3 l{ (1.0f - s) * luminance };
    return glm::vec3{
        dot(v, l + glm::vec3{s, 0.0f, 0.0f}),
        dot(v, l + glm::vec3{0.0f, s, 0.0f}),
        dot(v, l + glm::vec3{0.0f, 0.0f, s}),
    };

}


inline glm::vec3 curves(glm::vec3 v, glm::vec3 shadowGamma, glm::vec3 midPoint, glm::vec3 highlightScale) 
{
    // "Practical HDR and Wide Color Techniques in Gran Turismo SPORT", Uchimura 2018
    glm::vec3 d = 1.0f / (pow(midPoint, shadowGamma - 1.0f));
    glm::vec3 dark = pow(v, shadowGamma) * d;
    glm::vec3 light = highlightScale * (v - midPoint) + midPoint;
    return glm::vec3{
        v.r <= midPoint.r ? dark.r : light.r,
        v.g <= midPoint.g ? dark.g : light.g,
        v.b <= midPoint.b ? dark.b : light.b,
    };
}

//------------------------------------------------------------------------------
// Luminance scaling
//------------------------------------------------------------------------------

//static glm::vec3 luminanceScaling(glm::vec3 x,
//    const ToneMapper& toneMapper, glm::vec3 luminanceWeights) noexcept 
//{
//
//    // Troy Sobotka, 2021, "EVILS - Exposure Value Invariant Luminance Scaling"
//    // https://colab.research.google.com/drive/1iPJzNNKR7PynFmsqSnQm3bCZmQ3CvAJ-#scrollTo=psU43hb-BLzB
//
//    float luminanceIn = dot(x, luminanceWeights);
//
//    // TODO: We could optimize for the case of single-channel luminance
//    float luminanceOut = toneMapper(luminanceIn).y;
//
//    float peak = max(x);
//    glm::vec3 chromaRatio = max(x / peak, 0.0f);
//
//    float chromaRatioLuminance = dot(chromaRatio, luminanceWeights);
//
//    glm::vec3 maxReserves = 1.0f - chromaRatio;
//    float maxReservesLuminance = dot(maxReserves, luminanceWeights);
//
//    float luminanceDifference = std::max(luminanceOut - chromaRatioLuminance, 0.0f);
//    float scaledLuminanceDifference =
//        luminanceDifference / std::max(maxReservesLuminance, std::numeric_limits<float>::min());
//
//    float chromaScale = (luminanceOut - luminanceDifference) /
//        std::max(chromaRatioLuminance, std::numeric_limits<float>::min());
//
//    return chromaScale * chromaRatio + scaledLuminanceDifference * maxReserves;
//}

//------------------------------------------------------------------------------
// Quality
//------------------------------------------------------------------------------

static std::tuple<GLenum, GLenum, GLenum>
    selectLutTextureParams(CColorGradingPass::LutFormat lutFormat) noexcept
{
    // We use RGBA16F for high quality modes instead of RGB16F because RGB16F
    // is not supported everywhere
    switch (lutFormat) {
    case CColorGradingPass::LutFormat::INTEGER:
        return { GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV };
    case CColorGradingPass::LutFormat::FLOAT:
        return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT };
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
// The following functions exist to preserve backward compatibility with the
// `FILMIC` set via the deprecated `ToneMapping` API. Selecting `ToneMapping::FILMIC`
// forces post-processing to be performed in sRGB to guarantee that the inverse tone
// mapping function in the shaders will match the forward tone mapping step exactly.

static glm::mat3 selectColorGradingTransformIn(CColorGradingPass::ToneMapping toneMapping) noexcept
{
    if (toneMapping == CColorGradingPass::ToneMapping::FILMIC) {
        return glm::mat3{};
    }
    return sRGB_to_Rec2020;
}

static glm::mat3 selectColorGradingTransformOut(CColorGradingPass::ToneMapping toneMapping) noexcept
{
    if (toneMapping == CColorGradingPass::ToneMapping::FILMIC)
    {
        return glm::mat3{};
    }
    return Rec2020_to_sRGB;
}

static glm::vec3 selectColorGradingLuminance(CColorGradingPass::ToneMapping toneMapping) noexcept
{
    if (toneMapping == CColorGradingPass::ToneMapping::FILMIC)
    {
        return LUMINANCE_Rec709;
    }
    return LUMINANCE_Rec2020;
}

#pragma clang diagnostic pop
using ColorTransform = glm::vec3(*)(glm::vec3);

static ColorTransform selectOETF(const ColorSpace& colorSpace) noexcept 
{
    if (colorSpace.getTransferFunction() == Linear) 
    {
        return OETF_Linear;
    }
    return OETF_sRGB;
}

//------------------------------------------------------------------------------
// Color grading implementation
//------------------------------------------------------------------------------

struct Config 
{
    size_t lutDimension{};
    glm::mat3  adaptationTransform;
    glm::mat3  colorGradingIn;
    glm::mat3  colorGradingOut;
    glm::vec3 colorGradingLuminance{};
    ColorTransform oetf;
};



CColorGradingPass::CColorGradingPass(const std::string& vPassName, int vExcutionOrder) : IRenderPass(vPassName, vExcutionOrder)
{
    


}



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
void CColorGradingPass::initV()
{

    // Fallback for clients that still use the deprecated ToneMapping API
    bool needToneMapper = this->toneMapper == nullptr;
    if (needToneMapper)
    {
        switch (this->toneMapping)
        {
        case ToneMapping::LINEAR:
            this->toneMapper = new LinearToneMapper();
            break;
        case ToneMapping::ACES_LEGACY:
            this->toneMapper = new ACESLegacyToneMapper();
            break;
        case ToneMapping::ACES:
            this->toneMapper = new ACESToneMapper();
            break;
        case ToneMapping::FILMIC:
            this->toneMapper = new FilmicToneMapper();
            break;
        case ToneMapping::DISPLAY_RANGE:
            this->toneMapper = new DisplayRangeToneMapper();
            break;
        }
    }


    Config c;
    std::mutex configLock;
    {
        std::lock_guard<std::mutex> const lock(configLock);
        c.lutDimension = this->dimension;
        c.adaptationTransform = adaptationTransform(this->whiteBalance);
        c.colorGradingIn = selectColorGradingTransformIn(this->toneMapping);
        c.colorGradingOut = selectColorGradingTransformOut(this->toneMapping);
        c.colorGradingLuminance = selectColorGradingLuminance(this->toneMapping);
        c.oetf = selectOETF(this->outputColorSpace);
    }
    size_t mDimension = c.lutDimension;
    size_t lutElementCount = c.lutDimension * c.lutDimension * c.lutDimension;
    size_t elementSize = sizeof(glm::half4);

    void* data = malloc(lutElementCount * elementSize);

    auto [textureFormat, format, type] = selectLutTextureParams(this->format);


    void* converted = nullptr;
    if (type == GL_UNSIGNED_INT_2_10_10_10_REV)
    {
        // convert input to UINT_2_10_10_10_REV if needed
        converted = malloc(lutElementCount * sizeof(uint32_t));
    }


    std::vector<std::thread> js(c.lutDimension);
    for (size_t b = 0; b < c.lutDimension; b++)
    {
        js[b] = std::thread(
            [data, converted, b, &c, &configLock, this]()
            {
                Config config;
                {
                    std::lock_guard<std::mutex> lock(configLock);
                    config = c;
                }
                glm::half4* p = (glm::half4*)data + b * config.lutDimension * config.lutDimension;
                for (size_t g = 0; g < config.lutDimension; g++) {
                    for (size_t r = 0; r < config.lutDimension; r++) {
                        glm::vec3 v = glm::vec3{ r, g, b } *(1.0f / float(config.lutDimension - 1u));

                        // LogC encoding
                        v = LogC_to_linear(v);

                        // Kill negative values near 0.0f due to imprecision in the log conversion
                        v = max(v, 0.0f);

                        v = c.colorGradingIn * v;
                        v = (*this->toneMapper)(v);
                        // Apply gamut mapping
                        if (this->gamutMapping)
                        {
                            // TODO: This should depend on the output color space
                            v = gamutMapping_sRGB(v);
                        }

                        // TODO: We should convert to the output color space if we use a working
                        //       color space that's not sRGB
                        // TODO: Allow the user to customize the output color space

                        // We need to clamp for the output transfer function
                        v = glm::saturate(v);

                        // Apply OETF
                        v = c.oetf(v);

                        *p++ = glm::half4{ v, 0.0f };
                    }

                }

                if (converted) 
                {
                    uint32_t* const  dst = (uint32_t*)converted +
                        b * config.lutDimension * config.lutDimension;
                    glm::half4* src = (glm::half4*)data +
                        b * config.lutDimension * config.lutDimension;
                    // we use a vectorize width of 8 because, on ARMv8 it allows the compiler to write eight
                    // 32-bits results in one go.
                    const size_t count = (config.lutDimension * config.lutDimension) & ~0x7u; // tell the compiler that we're a multiple of 8
#pragma clang loop vectorize_width(8)
                    for (size_t i = 0; i < count; ++i) {
                        glm::vec3 v{ src[i] };
                        uint32_t pr = uint32_t(std::floor(v.x * 1023.0f + 0.5f));
                        uint32_t pg = uint32_t(std::floor(v.y * 1023.0f + 0.5f));
                        uint32_t pb = uint32_t(std::floor(v.z * 1023.0f + 0.5f));
                        dst[i] = (pb << 20u) | (pg << 10u) | pr;
                    }
                }
            }
        );
        js[b].join();
    }


    //std::chrono::duration<float, std::milli> duration = std::chrono::steady_clock::now() - now;
    //slog.d << "LUT generation time: " << duration.count() << " ms" << io::endl;


    auto mLutHandle = std::make_shared<ElayGraphics::STexture>();

    mLutHandle->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    mLutHandle->InternalFormat = textureFormat;
    mLutHandle->ExternalFormat = format;
    mLutHandle->DataType = type;
    mLutHandle->Width = c.lutDimension;
    mLutHandle->Height = c.lutDimension;
    mLutHandle->Depth = c.lutDimension;

    mLutHandle->pDataSet.resize(1);
    mLutHandle->pDataSet[0] = data;
  
    

    mLutHandle->Type4WrapS = GL_CLAMP_TO_BORDER;
    mLutHandle->Type4WrapT = GL_CLAMP_TO_BORDER;
    mLutHandle->BorderColor = { 0,0,0,0 };
    genTexture(mLutHandle);
    free(data);
    ElayGraphics::ResourceManager::registerSharedData("ColorGradingTexture", mLutHandle);

    if (needToneMapper)
    {
        delete this->toneMapper;
        this->toneMapper = nullptr;
    }
   

    m_pShader = std::make_shared<CShader>("ColorGrading_VS.glsl", "ColorGrading_FS.glsl");
    m_pShader->activeShader();
    std::shared_ptr<ElayGraphics::STexture> ComputeTexture = (ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("TextureConfig4Albedo"));
    m_pShader->setTextureUniformValue("u_Texture2D", ComputeTexture);

    m_pShader->setTextureUniformValue("u_Grading3D", mLutHandle);

}

void CColorGradingPass::updateV()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_pShader->activeShader();
    m_pShader->setFloatUniformValue("lutSize", 0.5f / this->dimension, (this->dimension - 1.0f) / this->dimension);
 
    drawQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
