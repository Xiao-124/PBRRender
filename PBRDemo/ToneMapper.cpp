
#include "ToneMapper.h"
#include "ColorSpaceUtils.h"



inline float rgb_2_saturation(glm::vec3 rgb) 
{
    // Input:  ACES
    // Output: OCES
    constexpr float TINY = 1e-5f;
    float mi = min(rgb);
    float ma = max(rgb);
    return (std::max(ma, TINY) - std::max(mi, TINY)) / std::max(ma, 1e-2f);
}

inline float rgb_2_yc(glm::vec3 rgb) 
{
    constexpr float ycRadiusWeight = 1.75f;

    // Converts RGB to a luminance proxy, here called YC
    // YC is ~ Y + K * Chroma
    // Constant YC is a cone-shaped surface in RGB space, with the tip on the
    // neutral axis, towards white.
    // YC is normalized: RGB 1 1 1 maps to YC = 1
    //
    // ycRadiusWeight defaults to 1.75, although can be overridden in function
    // call to rgb_2_yc
    // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral
    // of same value
    // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of
    // same value.

    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    float chroma = std::sqrt(b * (b - g) + g * (g - r) + r * (r - b));

    return (b + g + r + ycRadiusWeight * chroma) / 3.0f;
}

inline float sigmoid_shaper(float x) 
{
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.
    float t = std::max(1.0f - std::abs(x / 2.0f), 0.0f);
    float y = 1.0f + glm::sign(x) * (1.0f - t * t);
    return y / 2.0f;
}

inline float glow_fwd(float ycIn, float glowGainIn, float glowMid) 
{
    float glowGainOut;

    if (ycIn <= 2.0f / 3.0f * glowMid) 
    {
        glowGainOut = glowGainIn;
    }
    else if (ycIn >= 2.0f * glowMid) 
    {
        glowGainOut = 0.0f;
    }
    else 
    {
        glowGainOut = glowGainIn * (glowMid / ycIn - 1.0f / 2.0f);
    }

    return glowGainOut;
}

inline float rgb_2_hue(glm::vec3 rgb) 
{
    // Returns a geometric hue angle in degrees (0-360) based on RGB values.
    // For neutral colors, hue is undefined and the function will return a quiet NaN value.
    float hue = 0.0f;
    // RGB triplets where RGB are equal have an undefined hue
    if (!(rgb.x == rgb.y && rgb.y == rgb.z)) 
    {
        hue = glm::RAD_TO_DEG * std::atan2(
            std::sqrt(3.0f) * (rgb.y - rgb.z),
            2.0f * rgb.x - rgb.y - rgb.z);
    }
    return (hue < 0.0f) ? hue + 360.0f : hue;
}

inline float center_hue(float hue, float centerH) 
{
    float hueCentered = hue - centerH;
    if (hueCentered < -180.0f) 
    {
        hueCentered = hueCentered + 360.0f;
    }
    else if (hueCentered > 180.0f) 
    {
        hueCentered = hueCentered - 360.0f;
    }
    return hueCentered;
}

inline glm::vec3 darkSurround_to_dimSurround(glm::vec3 linearCV) 
{
    constexpr float DIM_SURROUND_GAMMA = 0.9811f;

    glm::vec3 XYZ = AP1_to_XYZ * linearCV;
    glm::vec3 xyY = XYZ_to_xyY(XYZ);

    xyY.z = glm::clamp(xyY.z, 0.0f, (float)std::numeric_limits<math::half>::max());
    xyY.z = std::pow(xyY.z, DIM_SURROUND_GAMMA);

    XYZ = xyY_to_XYZ(xyY);
    return XYZ_to_AP1 * XYZ;
}

glm::vec3 ACES(glm::vec3 color, float brightness) noexcept 
{
    // Some bits were removed to adapt to our desired output

    // "Glow" module constants
    constexpr float RRT_GLOW_GAIN = 0.05f;
    constexpr float RRT_GLOW_MID = 0.08f;

    // Red modifier constants
    constexpr float RRT_RED_SCALE = 0.82f;
    constexpr float RRT_RED_PIVOT = 0.03f;
    constexpr float RRT_RED_HUE = 0.0f;
    constexpr float RRT_RED_WIDTH = 135.0f;

    // Desaturation constants
    constexpr float RRT_SAT_FACTOR = 0.96f;
    constexpr float ODT_SAT_FACTOR = 0.93f;

    glm::vec3 ap0 = Rec2020_to_AP0 * color;

    // Glow module
    float saturation = rgb_2_saturation(ap0);
    float ycIn = rgb_2_yc(ap0);
    float s = sigmoid_shaper((saturation - 0.4f) / 0.2f);
    float addedGlow = 1.0f + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);
    ap0 *= addedGlow;

    // Red modifier
    float hue = rgb_2_hue(ap0);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight = glm::smoothstep(0.0f, 1.0f, 1.0f - std::abs(2.0f * centeredHue / RRT_RED_WIDTH));
    hueWeight *= hueWeight;

    ap0.r += hueWeight * saturation * (RRT_RED_PIVOT - ap0.r) * (1.0f - RRT_RED_SCALE);

    // ACES to RGB rendering space
    glm::vec3 ap1 = clamp(AP0_to_AP1 * ap0, 0.0f, (float)std::numeric_limits<math::half>::max());

    // Global desaturation
    ap1 = mix(glm::vec3(dot(ap1, LUMINANCE_AP1)), ap1, RRT_SAT_FACTOR);

    // NOTE: This is specific to Filament and added only to match ACES to our legacy tone mapper
    //       which was a fit of ACES in Rec.709 but with a brightness boost.
    ap1 *= brightness;

    // Fitting of RRT + ODT (RGB monitor 100 nits dim) from:
    // https://github.com/colour-science/colour-unity/blob/master/Assets/Colour/Notebooks/CIECAM02_Unity.ipynb
    constexpr float a = 2.785085f;
    constexpr float b = 0.107772f;
    constexpr float c = 2.936045f;
    constexpr float d = 0.887122f;
    constexpr float e = 0.806889f;
    glm::vec3 rgbPost = (ap1 * (a * ap1 + b)) / (ap1 * (c * ap1 + d) + e);

    // Apply gamma adjustment to compensate for dim surround
    glm::vec3 linearCV = darkSurround_to_dimSurround(rgbPost);

    // Apply desaturation to compensate for luminance difference
    linearCV = mix(glm::vec3(dot(linearCV, LUMINANCE_AP1)), linearCV, ODT_SAT_FACTOR);

    return AP1_to_Rec2020 * linearCV;
}


//------------------------------------------------------------------------------
// Tone mappers
//------------------------------------------------------------------------------

#define DEFAULT_CONSTRUCTORS(A) \
    A::A() noexcept = default; \
    A::~A() noexcept = default;

DEFAULT_CONSTRUCTORS(ToneMapper)

    //------------------------------------------------------------------------------
    // Linear tone mapper
    //------------------------------------------------------------------------------

    DEFAULT_CONSTRUCTORS(LinearToneMapper)

    glm::vec3 LinearToneMapper::operator()(glm::vec3 v) const noexcept {
    return saturate(v);
}

//------------------------------------------------------------------------------
// ACES tone mappers
//------------------------------------------------------------------------------

DEFAULT_CONSTRUCTORS(ACESToneMapper)

    glm::vec3 ACESToneMapper::operator()(glm::vec3 c) const noexcept 
{
    return ACES(c, 1.0f);
}

DEFAULT_CONSTRUCTORS(ACESLegacyToneMapper)
    glm::vec3 ACESLegacyToneMapper::operator()(glm::vec3 c) const noexcept 
{
    return ACES(c, 1.0f / 0.6f);
}

DEFAULT_CONSTRUCTORS(FilmicToneMapper)
    glm::vec3 FilmicToneMapper::operator()(glm::vec3 x) const noexcept 
{
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    constexpr float a = 2.51f;
    constexpr float b = 0.03f;
    constexpr float c = 2.43f;
    constexpr float d = 0.59f;
    constexpr float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

//------------------------------------------------------------------------------
// AgX tone mapper
//------------------------------------------------------------------------------

AgxToneMapper::AgxToneMapper(AgxToneMapper::AgxLook look) noexcept : look(look) {}
AgxToneMapper::~AgxToneMapper() noexcept = default;

// These matrices taken from Blender's implementation of AgX, which works with Rec.2020 primaries.
// https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBaseRec2020.py
glm::mat3 AgXInsetMatrix
{
    0.856627153315983, 0.137318972929847, 0.11189821299995,
    0.0951212405381588, 0.761241990602591, 0.0767994186031903,
    0.0482516061458583, 0.101439036467562, 0.811302368396859
};

glm::mat3 AgXOutsetMatrixInv
{
    0.899796955911611, 0.11142098895748, 0.11142098895748,
    0.0871996192028351, 0.875575586156966, 0.0871996192028349,
    0.013003424885555, 0.0130034248855548, 0.801379391839686
};
glm::mat3 AgXOutsetMatrix{ inverse(AgXOutsetMatrixInv) };

// LOG2_MIN      = -10.0
// LOG2_MAX      =  +6.5
// MIDDLE_GRAY   =  0.18
const float AgxMinEv = -12.47393f;      // log2(pow(2, LOG2_MIN) * MIDDLE_GRAY)
const float AgxMaxEv = 4.026069f;       // log2(pow(2, LOG2_MAX) * MIDDLE_GRAY)

// Adapted from https://iolite-engine.com/blog_posts/minimal_agx_implementation
glm::vec3 agxDefaultContrastApprox(glm::vec3 x) 
{
    glm::vec3 x2 = x * x;
    glm::vec3 x4 = x2 * x2;
    glm::vec3 x6 = x4 * x2;
    

    return  -17.86f * x6 * x
        + 78.01f * x6
        - 126.7f * x4 * x
        + 92.06f * x4
        - 28.72f * x2 * x
        + 4.361f * x2
        - 0.1718f * x
        + 0.002857f;
}

// Adapted from https://iolite-engine.com/blog_posts/minimal_agx_implementation
glm::vec3 agxLook(glm::vec3 val, AgxToneMapper::AgxLook look) 
{
    if (look == AgxToneMapper::AgxLook::NONE) 
    {
        return val;
    }
    const glm::vec3 lw = glm::vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(val, lw);

    // Default
    glm::vec3 offset = glm::vec3(0.0);
    glm::vec3 slope = glm::vec3(1.0);
    glm::vec3 power = glm::vec3(1.0);
    float sat = 1.0;

    if (look == AgxToneMapper::AgxLook::GOLDEN) 
    {
        slope = glm::vec3(1.0, 0.9, 0.5);
        power = glm::vec3(0.8);
        sat = 1.3;
    }
    if (look == AgxToneMapper::AgxLook::PUNCHY) 
    {
        slope = glm::vec3(1.0);
        power = glm::vec3(1.35, 1.35, 1.35);
        sat = 1.4;
    }

    // ASC CDL
    val = pow(val * slope + offset, power);
    return luma + sat * (val - luma);
}

glm::vec3 AgxToneMapper::operator()(glm::vec3 v) const noexcept 
{
    // Ensure no negative values
    v = max(glm::vec3(0.0), v);

    v = AgXInsetMatrix * v;

    // Log2 encoding
    v = glm::max(v, 1E-10f); // avoid 0 or negative numbers for log2
    v = log2(v);
    v = (v - AgxMinEv) / (AgxMaxEv - AgxMinEv);

    v = glm::clamp(v, 0.0f, 1.0f);

    // Apply sigmoid
    v = agxDefaultContrastApprox(v);

    // Apply AgX look
    v = agxLook(v, look);

    v = AgXOutsetMatrix * v;

    // Linearize
    v = pow(max(glm::vec3(0.0), v), 2.2);

    return v;
}

//------------------------------------------------------------------------------
// Display range tone mapper
//------------------------------------------------------------------------------

DEFAULT_CONSTRUCTORS(DisplayRangeToneMapper)

    glm::vec3 DisplayRangeToneMapper::operator()(glm::vec3 c) const noexcept 
{
    // 16 debug colors + 1 duplicated at the end for easy indexing
    constexpr glm::vec3 debugColors[17] = {
            {0.0,     0.0,     0.0},         // black
            {0.0,     0.0,     0.1647},      // darkest blue
            {0.0,     0.0,     0.3647},      // darker blue
            {0.0,     0.0,     0.6647},      // dark blue
            {0.0,     0.0,     0.9647},      // blue
            {0.0,     0.9255,  0.9255},      // cyan
            {0.0,     0.5647,  0.0},         // dark green
            {0.0,     0.7843,  0.0},         // green
            {1.0,     1.0,     0.0},         // yellow
            {0.90588, 0.75294, 0.0},         // yellow-orange
            {1.0,     0.5647,  0.0},         // orange
            {1.0,     0.0,     0.0},         // bright red
            {0.8392,  0.0,     0.0},         // red
            {1.0,     0.0,     1.0},         // magenta
            {0.6,     0.3333,  0.7882},      // purple
            {1.0,     1.0,     1.0},         // white
            {1.0,     1.0,     1.0}          // white
    };

    // The 5th color in the array (cyan) represents middle gray (18%)
    // Every stop above or below middle gray causes a color shift
    // TODO: This should depend on the working color grading color space
    float v = log2(dot(c, LUMINANCE_Rec2020) / 0.18f);
    v = glm::clamp(v + 5.0f, 0.0f, 15.0f);

    size_t index = size_t(v);
    return mix(debugColors[index], debugColors[index + 1], glm::saturate(v - float(index)));
}

//------------------------------------------------------------------------------
// Generic tone mapper
//------------------------------------------------------------------------------

struct GenericToneMapper::Options 
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
    void setParameters(
        float contrast,
        float midGrayIn,
        float midGrayOut,
        float hdrMax
    ) {
        contrast = std::max(contrast, 1e-5f);
        midGrayIn = glm::clamp(midGrayIn, 1e-5f, 1.0f);
        midGrayOut = glm::clamp(midGrayOut, 1e-5f, 1.0f);
        hdrMax = glm::max(hdrMax, 1.0f);

        this->contrast = contrast;
        this->midGrayIn = midGrayIn;
        this->midGrayOut = midGrayOut;
        this->hdrMax = hdrMax;

        float a = pow(midGrayIn, contrast);
        float b = pow(hdrMax, contrast);
        float c = a - midGrayOut * b;

        inputScale = (a * b * (midGrayOut - 1.0f)) / c;
        outputScale = midGrayOut * (a - b) / c;
    }
#pragma clang diagnostic pop

    float contrast;
    float midGrayIn;
    float midGrayOut;
    float hdrMax;

    // TEMP
    float inputScale;
    float outputScale;
};

GenericToneMapper::GenericToneMapper(
    float contrast,
    float midGrayIn,
    float midGrayOut,
    float hdrMax
) noexcept {
    mOptions = new Options();
    mOptions->setParameters(contrast, midGrayIn, midGrayOut, hdrMax);
}

GenericToneMapper::~GenericToneMapper() noexcept {
    delete mOptions;
}

GenericToneMapper::GenericToneMapper(GenericToneMapper&& rhs)  noexcept : mOptions(rhs.mOptions) {
    rhs.mOptions = nullptr;
}

GenericToneMapper& GenericToneMapper::operator=(GenericToneMapper&& rhs) noexcept {
    mOptions = rhs.mOptions;
    rhs.mOptions = nullptr;
    return *this;
}

glm::vec3 GenericToneMapper::operator()(glm::vec3 x) const noexcept 
{
    x = pow(x, mOptions->contrast);
    return mOptions->outputScale * x / (x + mOptions->inputScale);
}

float GenericToneMapper::getContrast() const noexcept { return  mOptions->contrast; }
float GenericToneMapper::getMidGrayIn() const noexcept { return  mOptions->midGrayIn; }
float GenericToneMapper::getMidGrayOut() const noexcept { return  mOptions->midGrayOut; }
float GenericToneMapper::getHdrMax() const noexcept { return  mOptions->hdrMax; }


void GenericToneMapper::setContrast(float contrast) noexcept 
{
    mOptions->setParameters(
        contrast,
        mOptions->midGrayIn,
        mOptions->midGrayOut,
        mOptions->hdrMax
    );
}

void GenericToneMapper::setMidGrayIn(float midGrayIn) noexcept 
{
    mOptions->setParameters(
        mOptions->contrast,
        midGrayIn,
        mOptions->midGrayOut,
        mOptions->hdrMax
    );
}

void GenericToneMapper::setMidGrayOut(float midGrayOut) noexcept 
{
    mOptions->setParameters(
        mOptions->contrast,
        mOptions->midGrayIn,
        midGrayOut,
        mOptions->hdrMax
    );
}

void GenericToneMapper::setHdrMax(float hdrMax) noexcept
{
    mOptions->setParameters(
        mOptions->contrast,
        mOptions->midGrayIn,
        mOptions->midGrayOut,
        hdrMax
    );
}

