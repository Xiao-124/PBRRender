
#include "Color.h"
#include "ColorSpaceUtils.h"




glm::vec3 Color::sRGBToLinear(glm::vec3 color) noexcept
{
    return EOTF_sRGB(color);
}

glm::vec3 Color::linearToSRGB(glm::vec3 color) noexcept 
{
    return OETF_sRGB(color);
}


LinearColor Color::cct(float K) 
{
    // temperature to CIE 1960
    float const K2 = K * K;
    float const u = (0.860117757f + 1.54118254e-4f * K + 1.28641212e-7f * K2) /
        (1.0f + 8.42420235e-4f * K + 7.08145163e-7f * K2);
    float const v = (0.317398726f + 4.22806245e-5f * K + 4.20481691e-8f * K2) /
        (1.0f - 2.89741816e-5f * K + 1.61456053e-7f * K2);

    float const d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
    glm::vec3 const linear = XYZ_to_sRGB * xyY_to_XYZ({ 3.0f * u * d, 2.0f * v * d, 1.0f });
    // normalize and saturate
    return saturate(linear / std::max(1e-5f, max(linear)));
}

LinearColor Color::illuminantD(float K) 
{
    // temperature to xyY
    const float iK = 1.0f / K;
    float const iK2 = iK * iK;
    float const x = K <= 7000.0f ?
        0.244063f + 0.09911e3f * iK + 2.9678e6f * iK2 - 4.6070e9f * iK2 * iK :
        0.237040f + 0.24748e3f * iK + 1.9018e6f * iK2 - 2.0064e9f * iK2 * iK;
    float const y = -3.0f * x * x + 2.87f * x - 0.275f;

    glm::vec3 const linear = XYZ_to_sRGB * xyY_to_XYZ({ x, y, 1.0f });
    // normalize and saturate
    return glm::saturate(linear / std::max(1e-5f, glm::max(linear)));
}

LinearColor Color::absorptionAtDistance(LinearColor const& color, float distance) 
{
    return -log(clamp(color, 1e-5f, 1.0f)) / std::max(1e-5f, distance);
}