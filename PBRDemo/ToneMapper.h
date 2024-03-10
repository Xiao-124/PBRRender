
#include <stdint.h>
#include <GLM/glm.hpp>

struct ToneMapper 
{
    ToneMapper() noexcept;
    virtual ~ToneMapper() noexcept;

    /**
        * Maps an open domain (or "scene referred" values) color value to display
        * domain (or "display referred") color value. Both the input and output
        * color values are defined in the Rec.2020 color space, with no transfer
        * function applied ("linear Rec.2020").
        *
        * @param c Input color to tone map, in the Rec.2020 color space with no
        *          transfer function applied ("linear")
        *
        * @return A tone mapped color in the Rec.2020 color space, with no transfer
        *         function applied ("linear")
        */
    virtual glm::vec3 operator()(glm::vec3 c) const noexcept = 0;
};

/**
    * Linear tone mapping operator that returns the input color but clamped to
    * the 0..1 range. This operator is mostly useful for debugging.
    */
struct LinearToneMapper final : public ToneMapper {
    LinearToneMapper() noexcept;
    ~LinearToneMapper() noexcept final;

    glm::vec3 operator()(glm::vec3 c) const noexcept override;
};

/**
    * ACES tone mapping operator. This operator is an implementation of the
    * ACES Reference Rendering Transform (RRT) combined with the Output Device
    * Transform (ODT) for sRGB monitors (dim surround, 100 nits).
    */
struct ACESToneMapper final : public ToneMapper 
{
    ACESToneMapper() noexcept;
    ~ACESToneMapper() noexcept final;

    glm::vec3 operator()(glm::vec3 c) const noexcept override;
};

/**
    * ACES tone mapping operator, modified to match the perceived brightness
    * of FilmicToneMapper. This operator is the same as ACESToneMapper but
    * applies a brightness multiplier of ~1.6 to the input color value to
    * target brighter viewing environments.
    */
struct ACESLegacyToneMapper final : public ToneMapper 
{
    ACESLegacyToneMapper() noexcept;
    ~ACESLegacyToneMapper() noexcept final;

    glm::vec3 operator()(glm::vec3 c) const noexcept override;
};

/**
    * "Filmic" tone mapping operator. This tone mapper was designed to
    * approximate the aesthetics of the ACES RRT + ODT for Rec.709
    * and historically Filament's default tone mapping operator. It exists
    * only for backward compatibility purposes and is not otherwise recommended.
    */
struct FilmicToneMapper final : public ToneMapper 
{
    FilmicToneMapper() noexcept;
    ~FilmicToneMapper() noexcept final;

    glm::vec3 operator()(glm::vec3 x) const noexcept override;
};

/**
    * AgX tone mapping operator.
    */
struct AgxToneMapper final : public ToneMapper
{

    enum class AgxLook : uint8_t 
    {
        NONE = 0,   //!< Base contrast with no look applied
        PUNCHY,     //!< A punchy and more chroma laden look for sRGB displays
        GOLDEN      //!< A golden tinted, slightly washed look for BT.1886 displays
    };

    /**
        * Builds a new AgX tone mapper.
        *
        * @param look an optional creative adjustment to contrast and saturation
        */
    explicit AgxToneMapper(AgxLook look = AgxLook::NONE) noexcept;
    ~AgxToneMapper() noexcept final;

    glm::vec3 operator()(glm::vec3 x) const noexcept override;

    AgxLook look;
};

/**
    * Generic tone mapping operator that gives control over the tone mapping
    * curve. This operator can be used to control the aesthetics of the final
    * image. This operator also allows to control the dynamic range of the
    * scene referred values.
    *
    * The tone mapping curve is defined by 5 parameters:
    * - contrast: controls the contrast of the curve
    * - midGrayIn: sets the input middle gray
    * - midGrayOut: sets the output middle gray
    * - hdrMax: defines the maximum input value that will be mapped to
    *           output white
    */
struct GenericToneMapper final : public ToneMapper 
{
    /**
        * Builds a new generic tone mapper. The default values of the
        * constructor parameters approximate an ACES tone mapping curve
        * and the maximum input value is set to 10.0.
        *
        * @param contrast controls the contrast of the curve, must be > 0.0, values
        *                 in the range 0.5..2.0 are recommended.
        * @param midGrayIn sets the input middle gray, between 0.0 and 1.0.
        * @param midGrayOut sets the output middle gray, between 0.0 and 1.0.
        * @param hdrMax defines the maximum input value that will be mapped to
        *               output white. Must be >= 1.0.
        */
    explicit GenericToneMapper(
        float contrast = 1.55f,
        float midGrayIn = 0.18f,
        float midGrayOut = 0.215f,
        float hdrMax = 10.0f
    ) noexcept;
    ~GenericToneMapper() noexcept final;

    GenericToneMapper(GenericToneMapper const&) = delete;
    GenericToneMapper& operator=(GenericToneMapper const&) = delete;
    GenericToneMapper(GenericToneMapper&& rhs)  noexcept;
    GenericToneMapper& operator=(GenericToneMapper&& rhs) noexcept;

    glm::vec3 operator()(glm::vec3 x) const noexcept override;

    /** Returns the contrast of the curve as a strictly positive value. */
    float getContrast() const noexcept;

    /** Returns how fast scene referred values map to output white as a value between 0.0 and 1.0. */
    float getShoulder() const noexcept;

    /** Returns the middle gray point for input values as a value between 0.0 and 1.0. */
    float getMidGrayIn() const noexcept;

    /** Returns the middle gray point for output values as a value between 0.0 and 1.0. */
    float getMidGrayOut() const noexcept;

    /** Returns the maximum input value that will map to output white, as a value >= 1.0. */
    float getHdrMax() const noexcept;

    /** Sets the contrast of the curve, must be > 0.0, values in the range 0.5..2.0 are recommended. */
    void setContrast(float contrast) noexcept;

    /** Sets the input middle gray, between 0.0 and 1.0. */
    void setMidGrayIn(float midGrayIn) noexcept;

    /** Sets the output middle gray, between 0.0 and 1.0. */
    void setMidGrayOut(float midGrayOut) noexcept;

    /** Defines the maximum input value that will be mapped to output white. Must be >= 1.0. */
    void setHdrMax(float hdrMax) noexcept;

private:
    struct Options;
    Options* mOptions;
};

/**
    * A tone mapper that converts the input HDR RGB color into one of 16 debug colors
    * that represent the pixel's exposure. When the output is cyan, the input color
    * represents  middle gray (18% exposure). Every exposure stop above or below middle
    * gray causes a color shift.
    *
    * The relationship between exposures and colors is:
    *
    * - -5EV  black
    * - -4EV  darkest blue
    * - -3EV  darker blue
    * - -2EV  dark blue
    * - -1EV  blue
    * -  OEV  cyan
    * - +1EV  dark green
    * - +2EV  green
    * - +3EV  yellow
    * - +4EV  yellow-orange
    * - +5EV  orange
    * - +6EV  bright red
    * - +7EV  red
    * - +8EV  magenta
    * - +9EV  purple
    * - +10EV white
    *
    * This tone mapper is useful to validate and tweak scene lighting.
    */
struct DisplayRangeToneMapper final : public ToneMapper {
    DisplayRangeToneMapper() noexcept;
    ~DisplayRangeToneMapper() noexcept override;

    glm::vec3 operator()(glm::vec3 c) const noexcept override;
};



