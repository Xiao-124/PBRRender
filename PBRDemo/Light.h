#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Color.h"


enum class IntensityUnit 
{
    LUMEN_LUX,  // intensity specified in lumens (for punctual lights) or lux (for directional)
    CANDELA     // intensity specified in candela (only applicable to punctual lights)
};


class CLight
{
public:
    enum class Type : uint8_t 
    {
        SUN,            //!< Directional light that also draws a sun's disk in the sky.
        DIRECTIONAL,    //!< Directional light, emits light in a given direction.
        POINT,          //!< Point light, emits light from a position, in all directions.
        FOCUSED_SPOT,   //!< Physically correct spot light.
        SPOT,           //!< Spot light with coupling of outer cone and illumination disabled.
    };
    void setPosition(const glm::vec3& position) noexcept;
    void setColor(const LinearColor& color);
    void setDirection(const glm::vec3& direction);
    void setSunAngularRadius(float angularRadius);
    void setIntensity(float intensity);
    void setSunHaloSize(float haloSize) noexcept;
    void setSunHaloFalloff(float haloFalloff) noexcept;
    static constexpr float EFFICIENCY_INCANDESCENT = 0.0220f;   //!< Typical efficiency of an incandescent light bulb (2.2%)
    static constexpr float EFFICIENCY_HALOGEN = 0.0707f;   //!< Typical efficiency of an halogen light bulb (7.0%)
    static constexpr float EFFICIENCY_FLUORESCENT = 0.0878f;   //!< Typical efficiency of a fluorescent light bulb (8.7%)
    static constexpr float EFFICIENCY_LED = 0.1171f;   //!< Typical efficiency of a LED light bulb (11.7%)
    
    Type getType() const noexcept;

    const glm::vec3& getColor() const noexcept 
    {
        return mColor;
    }

    float getIntensity() const noexcept 
    {
        return mIntensity;
    }

    float getSunAngularRadius() const noexcept {

        return  sunAngularRadius;
    }

    float getSunHaloSize() const noexcept 
    {
        return mSunHaloSize;
    }

    float getSunHaloFalloff() const noexcept 
    {
        return mSunHaloFalloff;
    }

    const glm::vec3& getLocalPosition() const noexcept
    {
        return mPosition;
    }

    const glm::vec3& getLocalDirection() const noexcept
    {
        return mDirection;
    }

protected:
    Type mType = Type::DIRECTIONAL;
    bool mCastShadows = false;
    bool mCastLight = true;
    uint8_t mChannels = 1u;

    glm::vec3 mPosition = {};
    float mFalloff = 1.0f;
    LinearColor mColor = LinearColor{ 1.0f };
    float mIntensity = 100000.0f;

    IntensityUnit mIntensityUnit = IntensityUnit::LUMEN_LUX;
    glm::vec3 mDirection = { 0.0f, -1.0f, 0.0f };

    glm::vec2 mSpotInnerOuter;
    float mSunAngle = 0.00951f; // 0.545бу in radians
    float mSunHaloSize = 10.0f;
    float mSunHaloFalloff = 80.0f;
    float sunAngularRadius;
};

