#include "Light.h"

void CLight::setPosition(const glm::vec3& position) noexcept
{
    mPosition = position;
}

void CLight::setColor(const LinearColor& color)
{
    mColor = color;
}


void CLight::setDirection(const glm::vec3& direction)
{
    mDirection = direction;
}

void CLight::setSunAngularRadius(float angularRadius)
{
    angularRadius = glm::clamp(angularRadius, 0.25f, 20.0f);
    sunAngularRadius = angularRadius * glm::DEG_TO_RAD;

}

void CLight::setIntensity(float intensity)
{
    mIntensity = intensity;
}

void CLight::setSunHaloSize(float haloSize) noexcept
{
    mSunHaloSize = haloSize;
}

void CLight::setSunHaloFalloff(float haloFalloff) noexcept
{
    mSunHaloFalloff = haloFalloff;
}




CLight::Type CLight::getType() const noexcept
{
    return mType;
}


