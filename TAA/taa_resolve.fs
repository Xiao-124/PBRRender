#version 450 core

layout(binding=0) uniform sampler2D currentColor;
layout(binding=1) uniform sampler2D previousColor;
layout(binding=2) uniform sampler2D velocityTexture;
layout(binding=3) uniform sampler2D currentDepth;

uniform float ScreenWidth;
uniform float ScreenHeight;
uniform int frameCount;

in  vec2 screenPosition;
out vec4 outColor;

vec3 RGB2YCoCgR(vec3 rgbColor)
{
    vec3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2;

    return YCoCgRColor;
}

vec3 YCoCgR2RGB(vec3 YCoCgRColor)
{
    vec3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

float Luminance(vec3 color)
{
    return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

vec3 ToneMap(vec3 color)
{
    return color / (1 + Luminance(color));
}

vec3 UnToneMap(vec3 color)
{
    return color / (1 - Luminance(color));
}


vec2 getClosestOffset()
{
    vec2 deltaRes = vec2(1.0 / ScreenWidth, 1.0 / ScreenHeight);
    float closestDepth = 1.0f;
    vec2 closestUV = screenPosition;

    for(int i=-1;i<=1;++i)
    {
        for(int j=-1;j<=1;++j)
        {
            vec2 newUV = screenPosition + deltaRes * vec2(i, j);

            float depth = texture2D(currentDepth, newUV).x;

            if(depth < closestDepth)
            {
                closestDepth = depth;
                closestUV = newUV;
            }
        }
    }

    return closestUV;
}

vec3 clipAABB(vec3 nowColor, vec3 preColor)
{
    vec3 aabbMin = nowColor, aabbMax = nowColor;
    vec2 deltaRes = vec2(1.0 / ScreenWidth, 1.0 / ScreenHeight);
    vec3 m1 = vec3(0), m2 = vec3(0);

    for(int i=-1;i<=1;++i)
    {
        for(int j=-1;j<=1;++j)
        {
            vec2 newUV = screenPosition + deltaRes * vec2(i, j);
            vec3 C = RGB2YCoCgR(ToneMap(texture(currentColor, newUV).rgb));
            m1 += C;
            m2 += C * C;
        }
    }

    // Variance clip
    const int N = 9;
    const float VarianceClipGamma = 1.0f;
    vec3 mu = m1 / N;
    vec3 sigma = sqrt(abs(m2 / N - mu * mu));
    aabbMin = mu - VarianceClipGamma * sigma;
    aabbMax = mu + VarianceClipGamma * sigma;
	 // clip to center
    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin);

    vec3 v_clip = preColor - p_clip;
    vec3 v_unit = v_clip.xyz / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return p_clip + v_clip / ma_unit;
    else
        return preColor;
}


vec4 sampleTextureCatmullRom(const sampler2D tex, const highp vec2 uv, const highp vec2 texSize) 
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate.
    // We'll do this by rounding down the sample location to get the exact center of our "starting"
    // texel. The starting texel will be at location [1, 1] in the grid, where [0, 0] is the
    // top left corner.

    highp vec2 samplePos = uv * texSize;
    highp vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location,
    // which we'll feed into the Catmull-Rom spline function to get our filter weights.
    highp vec2 f = samplePos - texPos1;
    highp vec2 f2 = f * f;
    highp vec2 f3 = f2 * f;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    vec2 w0 = f2 - 0.5 * (f3 + f);
    vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    vec2 w3 = 0.5 * (f3 - f2);
    vec2 w2 = 1.0 - w0 - w1 - w3;

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    vec2 w12 = w1 + w2;

    // Compute the final UV coordinates we'll use for sampling the texture
    highp vec2 texPos0 = texPos1 - vec2(1.0);
    highp vec2 texPos3 = texPos1 + vec2(2.0);
    highp vec2 texPos12 = texPos1 + w2 / w12;

    highp vec2 invTexSize = 1.0 / texSize;
    texPos0  *= invTexSize;
    texPos3  *= invTexSize;
    texPos12 *= invTexSize;

    float k0 = w12.x * w0.y;
    float k1 = w0.x  * w12.y;
    float k2 = w12.x * w12.y;
    float k3 = w3.x  * w12.y;
    float k4 = w12.x * w3.y;

    vec4 s[5];
    s[0] = textureLod(tex, vec2(texPos12.x, texPos0.y),  0.0);
    s[1] = textureLod(tex, vec2(texPos0.x,  texPos12.y), 0.0);
    s[2] = textureLod(tex, vec2(texPos12.x, texPos12.y), 0.0);
    s[3] = textureLod(tex, vec2(texPos3.x,  texPos12.y), 0.0);
    s[4] = textureLod(tex, vec2(texPos12.x, texPos3.y),  0.0);

    vec4 result =   k0 * s[0]
                  + k1 * s[1]
                  + k2 * s[2]
                  + k3 * s[3]
                  + k4 * s[4];

    result *= rcp(k0 + k1 + k2 + k3 + k4);

    // we could end-up with negative values
    result = max(vec4(0), result);

    return result;
}
void main()
{
    vec3 nowColor = texture(currentColor, screenPosition).rgb;
    if(frameCount == 0)
    {
        outColor = vec4(nowColor, 1.0);
        return;
    }

    // 周围3x3内距离最近的速度向量
    vec2 velocity = texture(velocityTexture, getClosestOffset()).rg;
    vec2 offsetUV = clamp(screenPosition - velocity, 0, 1);


    //普通采样
    vec3 preColor = texture(previousColor, offsetUV).rgb;

    //CatmullRom滤波采样
    vec4 preColor = sampleTextureCatmullRom(previousColor, offsetUV， vec2(textureSize(previousColor, 0));


    nowColor = RGB2YCoCgR(ToneMap(nowColor));
    preColor = RGB2YCoCgR(ToneMap(preColor));
	//
    preColor = clipAABB(nowColor, preColor);
	//
    preColor = UnToneMap(YCoCgR2RGB(preColor));
    nowColor = UnToneMap(YCoCgR2RGB(nowColor));


    float c = 0.05;
    outColor = vec4(c * nowColor + (1-c) * preColor, 1.0);
	//outColor = vec4(nowColor, 1.0);
	
}