#pragma once

#include <iostream>
#include <GLM/glm.hpp>
#include <cmath>
#include "glmextend.h"


using LinearColor = glm::vec3;
using sRGBColor = glm::vec3;

using LinearColorA = glm::vec4;
using sRGBColorA = glm::vec4;

//! types of RGB colors
enum class RgbType : uint8_t 
{
    sRGB,   //!< the color is defined in Rec.709-sRGB-D65 (sRGB) space
    LINEAR, //!< the color is defined in Rec.709-Linear-D65 ("linear sRGB") space
};

//! types of RGBA colors
enum class RgbaType : uint8_t 
{
    /**
        * the color is defined in Rec.709-sRGB-D65 (sRGB) space and the RGB values
        * have not been pre-multiplied by the alpha (for instance, a 50%
        * transparent red is <1,0,0,0.5>)
        */
    sRGB,
    /**
        * the color is defined in Rec.709-Linear-D65 ("linear sRGB") space and the
        * RGB values have not been pre-multiplied by the alpha (for instance, a 50%
        * transparent red is <1,0,0,0.5>)
        */
    LINEAR,
    /**
        * the color is defined in Rec.709-sRGB-D65 (sRGB) space and the RGB values
        * have been pre-multiplied by the alpha (for instance, a 50%
        * transparent red is <0.5,0,0,0.5>)
        */
    PREMULTIPLIED_sRGB,
    /**
        * the color is defined in Rec.709-Linear-D65 ("linear sRGB") space and the
        * RGB values have been pre-multiplied by the alpha (for instance, a 50%
        * transparent red is <0.5,0,0,0.5>)
        */
    PREMULTIPLIED_LINEAR
};

//! type of color conversion to use when converting to/from sRGB and linear spaces
enum ColorConversion 
{
    ACCURATE,   //!< accurate conversion using the sRGB standard
    FAST        //!< fast conversion using a simple gamma 2.2 curve
};



/**
    * Utilities to manipulate and convert colors
    */
class Color 
{
public:
    //! converts an RGB color to linear space, the conversion depends on the specified type
    static LinearColor toLinear(RgbType type, glm::vec3 color);

    //! converts an RGBA color to linear space, the conversion depends on the specified type
    static LinearColorA toLinear(RgbaType type, glm::vec4 color);

    //! converts an RGB color in sRGB space to an RGB color in linear space
    template<ColorConversion = ACCURATE>
    static LinearColor toLinear(sRGBColor const& color);

    /**
        * Converts an RGB color in Rec.709-Linear-D65 ("linear sRGB") space to an
        * RGB color in Rec.709-sRGB-D65 (sRGB) space.
        */
    template<ColorConversion = ACCURATE>
    static sRGBColor toSRGB(LinearColor const& color);

    /**
        * Converts an RGBA color in Rec.709-sRGB-D65 (sRGB) space to an RGBA color in
        * Rec.709-Linear-D65 ("linear sRGB") space the alpha component is left unmodified.
        */
    template<ColorConversion = ACCURATE>
    static LinearColorA toLinear(sRGBColorA const& color);

    /**
        * Converts an RGBA color in Rec.709-Linear-D65 ("linear sRGB") space to
        * an RGBA color in Rec.709-sRGB-D65 (sRGB) space the alpha component is
        * left unmodified.
        */
    template<ColorConversion = ACCURATE>
    static sRGBColorA toSRGB(LinearColorA const& color);

    /**
        * Converts a correlated color temperature to a linear RGB color in sRGB
        * space the temperature must be expressed in kelvin and must be in the
        * range 1,000K to 15,000K.
        */
    static LinearColor cct(float K);

    /**
        * Converts a CIE standard illuminant series D to a linear RGB color in
        * sRGB space the temperature must be expressed in kelvin and must be in
        * the range 4,000K to 25,000K
        */
    static LinearColor illuminantD(float K);

    /**
        * Computes the Beer-Lambert absorption coefficients from the specified
        * transmittance color and distance. The computed absorption will guarantee
        * the white light will become the specified color at the specified distance.
        * The output of this function can be used as the absorption parameter of
        * materials that use refraction.
        *
        * @param color the desired linear RGB color in sRGB space
        * @param distance the distance at which white light should become the specified color
        *
        * @return absorption coefficients for the Beer-Lambert law
        */
    static glm::vec3 absorptionAtDistance(LinearColor const& color, float distance);

private:
    static glm::vec3 sRGBToLinear(glm::vec3 color) noexcept;
    static glm::vec3 linearToSRGB(glm::vec3 color) noexcept;
};




// Use the default implementation from the header
template<>
inline LinearColor Color::toLinear<FAST>(sRGBColor const& color) 
{
    return glm::pow(color, 2.2f);
}

template<>
inline LinearColorA Color::toLinear<FAST>(sRGBColorA const& color) 
{
    return LinearColorA{ glm::pow(glm::vec3(color.r, color.g, color.b), 2.2f), color.a };
}

template<>
inline LinearColor Color::toLinear<ACCURATE>(sRGBColor const& color) 
{
    return sRGBToLinear(color);
}

template<>
inline LinearColorA Color::toLinear<ACCURATE>(sRGBColorA const& color) 
{
    return LinearColorA{ sRGBToLinear(glm::vec3(color.r, color.g, color.b)), color.a };
}

// Use the default implementation from the header
template<>
inline sRGBColor Color::toSRGB<FAST>(LinearColor const& color) 
{
    return glm::pow(color, 1.0f / 2.2f);
}

template<>
inline sRGBColorA Color::toSRGB<FAST>(LinearColorA const& color) 
{
    return sRGBColorA{ glm::pow(glm::vec3(color.r, color.g, color.b), 1.0f / 2.2f), color.a };
}

template<>
inline sRGBColor Color::toSRGB<ACCURATE>(LinearColor const& color) 
{
    return linearToSRGB(color);
}

template<>
inline sRGBColorA Color::toSRGB<ACCURATE>(LinearColorA const& color) 
{
    return sRGBColorA{ linearToSRGB(glm::vec3(color.r, color.g, color.b)), color.a };
}

inline LinearColor Color::toLinear(RgbType type, glm::vec3 color)
{
    return (type == RgbType::LINEAR) ? color : Color::toLinear<ACCURATE>(color);
}

// converts an RGBA color to linear space
// the conversion depends on the specified type
inline LinearColorA Color::toLinear(RgbaType type, glm::vec4 color)
{
    switch (type) {
    case RgbaType::sRGB:
        return Color::toLinear<ACCURATE>(color) * glm::vec4{ color.a, color.a, color.a, 1.0f };
    case RgbaType::LINEAR:
        return color * glm::vec4{ color.a, color.a, color.a, 1.0f };
    case RgbaType::PREMULTIPLIED_sRGB:
        return Color::toLinear<ACCURATE>(color);
    case RgbaType::PREMULTIPLIED_LINEAR:
        return color;
    }
}




