
#pragma once


#include <stdint.h>
#include <stddef.h>
#include <GLM/glm.hpp>
#include "ColorSpace.h"
#include "RenderPass.h"
#include "ToneMapper.h"
//class ToneMapper;


struct AgxToneMapperSettings
{
    AgxToneMapper::AgxLook look = AgxToneMapper::AgxLook::NONE;
    bool operator!=(const AgxToneMapperSettings& rhs) const { return !(rhs == *this); }
    bool operator==(const AgxToneMapperSettings& rhs) const;
};

struct GenericToneMapperSettings
{
    float contrast = 1.55f;
    float midGrayIn = 0.18f;
    float midGrayOut = 0.215f;
    float hdrMax = 10.0f;
    bool operator!=(const GenericToneMapperSettings& rhs) const { return !(rhs == *this); }
    bool operator==(const GenericToneMapperSettings& rhs) const;
};



class CColorGradingPass : public IRenderPass
{
public:
    CColorGradingPass(const std::string& vPassName, int vExcutionOrder);
    ~CColorGradingPass() = default;
    enum class QualityLevel : uint8_t 
    {
        LOW,
        MEDIUM,
        HIGH,
        ULTRA
    };

    enum class LutFormat : uint8_t 
    {
        INTEGER,    //!< 10 bits per component
        FLOAT,      //!< 16 bits per component (10 bits mantissa precision)
    };


    /**
        * List of available tone-mapping operators.
        *
        * @deprecated Use Builder::toneMapper(ToneMapper*) instead
        */
    /*
    LINEAR = 0,
    ACES_LEGACY = 1,
    ACES = 2,
    FILMIC = 3,
    AGX = 4,
    GENERIC = 5,
    DISPLAY_RANGE = 6,
    */
    enum class ToneMapping : uint8_t 
    {
        LINEAR = 0,     //!< Linear tone mapping (i.e. no tone mapping)
        ACES_LEGACY = 1,     //!< ACES tone mapping, with a brightness modifier to match Filament's legacy tone mapper
        ACES = 2,     //!< ACES tone mapping
        FILMIC = 3,     //!< Filmic tone mapping, modelled after ACES but applied in sRGB space
        AGX = 4,
        GENERIC = 5,
        DISPLAY_RANGE = 6,     //!< Tone mapping used to validate/debug scene exposure

    };

    void setQuality(QualityLevel qualityLevel) noexcept;
    void setFormat(LutFormat format) noexcept; 
    void setDimensions(uint8_t dim) noexcept;
    void setToneMapper(ToneMapper const* toneMapper) noexcept;
    void setLuminanceScaling(bool luminanceScaling) noexcept;
    void setGamutMapping(bool gamutMapping) noexcept;
    void setExposure(float exposure) noexcept;
    void setNightAdaptation(float adaptation) noexcept;
    void setWhiteBalance(float temperature, float tint) noexcept;
    void setChannelMixer(glm::vec3 outRed, glm::vec3 outGreen, glm::vec3 outBlue) noexcept;
    void setShadowsMidtonesHighlights(
            glm::vec4 shadows, glm::vec4 midtones, glm::vec4 highlights,
            glm::vec4 ranges) noexcept;
    void setSlopeOffsetPower(glm::vec3 slope, glm::vec3 offset, glm::vec3 power) noexcept;

    void setContrast(float contrast) noexcept;
    void setVibrance(float vibrance) noexcept;
    void setSaturation(float saturation) noexcept;
    void setCurves(glm::vec3 shadowGamma, glm::vec3 midPoint, glm::vec3 highlightScale) noexcept;
    void setOutputColorSpace(const ColorSpace& colorSpace) noexcept;

    virtual void initV();
    virtual void updateV();


private:
    const ToneMapper* toneMapper = nullptr;
    AgxToneMapperSettings agxToneMapperSetting;
    GenericToneMapperSettings genericToneMapperSetting;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ToneMapping toneMapping = ToneMapping::ACES_LEGACY;
#pragma clang diagnostic pop

    bool hasAdjustments = false;

    // Everything below must be part of the == comparison operator
    LutFormat format = LutFormat::FLOAT;
    uint8_t dimension = 32;

    // Out-of-gamut color handling
    bool   luminanceScaling = false;
    bool   gamutMapping = false;
    // Exposure
    float  exposure = 0.0f;
    // Night adaptation
    float  nightAdaptation = 0.0f;
    // White balance
    glm::vec2 whiteBalance = { 0.0f, 0.0f };
    // Channel mixer
    glm::vec3 outRed = { 1.0f, 0.0f, 0.0f };
    glm::vec3 outGreen = { 0.0f, 1.0f, 0.0f };
    glm::vec3 outBlue = { 0.0f, 0.0f, 1.0f };
    // Tonal ranges
    glm::vec3 shadows = { 1.0f, 1.0f, 1.0f };
    glm::vec3 midtones = { 1.0f, 1.0f, 1.0f };
    glm::vec3 highlights = { 1.0f, 1.0f, 1.0f };
    glm::vec4 tonalRanges = { 0.0f, 0.333f, 0.550f, 1.0f }; // defaults in DaVinci Resolve
    // ASC CDL
    glm::vec3 slope = { 1.0f, 1.0f, 1.0f };
    glm::vec3 offset = { 0.0f,0.0f,0.0f };
    glm::vec3 power = { 1.0f, 1.0f, 1.0f };
    // Color adjustments
    float  contrast = 1.0f;
    float  vibrance = 1.0f;
    float  saturation = 1.0f;
    // Curves
    glm::vec3 shadowGamma = { 1.0f, 1.0f, 1.0f };
    glm::vec3 midPoint = { 1.0f, 1.0f, 1.0f };
    glm::vec3 highlightScale = { 1.0f, 1.0f, 1.0f };

    // Output color space
    ColorSpace outputColorSpace = Rec709 - sRGB - D65;
    
    std::shared_ptr<CShader> taaShader;
    GLuint taaFBO;
    std::shared_ptr<ElayGraphics::STexture> histroyTexture;

protected:
    // prevent heap allocation
    
};




enum class ToneMappingSeeting : uint8_t
{
    LINEAR = 0,
    ACES_LEGACY = 1,
    ACES = 2,
    FILMIC = 3,
    AGX = 4,
    GENERIC = 5,
    DISPLAY_RANGE = 6,
};



struct ColorGradingSettings
{
    // fields are ordered to avoid padding
    bool enabled = true;
    bool linkedCurves = false;
    bool luminanceScaling = false;
    bool gamutMapping = false;
    CColorGradingPass::QualityLevel quality = CColorGradingPass::QualityLevel::MEDIUM;
    ToneMappingSeeting toneMapping = ToneMappingSeeting::ACES_LEGACY;
    bool padding0{};
    AgxToneMapperSettings agxToneMapper;
    ColorSpace colorspace = Rec709 - sRGB - D65;
    GenericToneMapperSettings genericToneMapper;
    glm::vec4 shadows{ 1.0f, 1.0f, 1.0f, 0.0f };
    glm::vec4 midtones{ 1.0f, 1.0f, 1.0f, 0.0f };
    glm::vec4 highlights{ 1.0f, 1.0f, 1.0f, 0.0f };
    glm::vec4 ranges{ 0.0f, 0.333f, 0.550f, 1.0f };
    glm::vec3 outRed{ 1.0f, 0.0f, 0.0f };
    glm::vec3 outGreen{ 0.0f, 1.0f, 0.0f };
    glm::vec3 outBlue{ 0.0f, 0.0f, 1.0f };
    glm::vec3 slope{ 1.0f };
    glm::vec3 offset{ 0.0f };
    glm::vec3 power{ 1.0f };
    glm::vec3 gamma{ 1.0f };
    glm::vec3 midPoint{ 1.0f };
    glm::vec3 scale{ 1.0f };
    float exposure = 0.0f;
    float nightAdaptation = 0.0f;
    float temperature = 0.0f;
    float tint = 0.0f;
    float contrast = 1.0f;
    float vibrance = 1.0f;
    float saturation = 1.0f;

    bool operator!=(const ColorGradingSettings& rhs) const { return !(rhs == *this); }
    bool operator==(const ColorGradingSettings& rhs) const;
};