#include "IBL.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <Interface.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Utils.h"
#include "Shader.h"
#include "Common.h"
#include "glmextend.h"
#include <Basetsd.h>


IBLLigthPass::IBLLigthPass(const std::string& vPassName, int vExcutionOrder) : IRenderPass(vPassName, vExcutionOrder)
{

}

IBLLigthPass::~IBLLigthPass()
{

}

void IBLLigthPass::setIBLPath(const std::string& vIBLPath)
{
    IBLPath = vIBLPath;
}

template<typename T>
static inline constexpr T log4(T x)
{
    return std::log2(x) * T(0.5);
}


typedef SSIZE_T ssize_t;

static inline float sphereQuadrantArea(float x, float y) 
{
    return std::atan2(x * y, std::sqrt(x * x + y * y + 1));
}

float solidAngle(size_t dim, size_t u, size_t v) 
{
    const float iDim = 1.0f / dim;
    float s = ((u + 0.5f) * 2 * iDim) - 1;
    float t = ((v + 0.5f) * 2 * iDim) - 1;
    const float x0 = s - iDim;
    const float y0 = t - iDim;
    const float x1 = s + iDim;
    const float y1 = t + iDim;
    float solidAngle = sphereQuadrantArea(x0, y0) -
        sphereQuadrantArea(x0, y1) -
        sphereQuadrantArea(x1, y0) +
        sphereQuadrantArea(x1, y1);
    return solidAngle;
}

inline glm::vec3 getDirectionFor(GLenum face, float x, float y) 
{
    // map [0, dim] to [-1,1] with (-1,-1) at bottom left

    float mScale = 2.0f / 256.0f;;
    float cx = (x * mScale) - 1;
    float cy = 1 - (y * mScale);

    glm::vec3 dir;
    const float l = std::sqrt(cx * cx + cy * cy + 1);
    switch (face) 
    {
        
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:  dir = { 1, cy,  -cx }; break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:  dir = { -1, cy,  cx }; break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:  dir = { cx,  1, -cy }; break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:  dir = { cx, -1,  cy }; break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:  dir = { cx, cy,   1 }; break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:  dir = { -cx, cy, -1 }; break;
    }
    return dir * (1.0f / l);
}


static inline constexpr size_t SHindex(ssize_t m, size_t l)
{
    return l * (l + 1) + m;
}

static constexpr float factorial(size_t n, size_t d = 1) 
{
    d = std::max(size_t(1), d);
    n = std::max(size_t(1), n);
    float r = 1.0;
    if (n == d) 
    {
        // intentionally left blank
    }
    else if (n > d) 
    {
        for (; n > d; n--) 
        {
            r *= n;
        }
    }
    else 
    {
        for (; d > n; d--) 
        {
            r *= d;
        }
        r = 1.0f / r;
    }
    return r;
}
constexpr const double F_2_SQRTPI = 1.12837916709551257389615890312154517;
constexpr const double F_SQRT2 = 1.41421356237309504880168872420969808;

float Kml(ssize_t m, size_t l) 
{
    m = m < 0 ? -m : m;  // abs() is not constexpr
    const float K = (2 * l + 1) * factorial(size_t(l - m), size_t(l + m));
    return std::sqrt(K) * (F_2_SQRTPI * 0.25);
}

std::vector<float> Ki(size_t numBands) 
{
    const size_t numCoefs = numBands * numBands;
    std::vector<float> K(numCoefs);
    for (size_t l = 0; l < numBands; l++) {
        K[SHindex(0, l)] = Kml(0, l);
        for (ssize_t m = 1; m <= l; m++) {
            K[SHindex(m, l)] =
                K[SHindex(-m, l)] = F_SQRT2 * Kml(m, l);
        }
    }
    return K;
}

constexpr const double F_PI = 3.14159265358979323846264338327950288;
constexpr float computeTruncatedCosSh(size_t l) 
{
    if (l == 0) 
    {
        return F_PI;
    }
    else if (l == 1) 
    {
        return 2 * F_PI / 3;
    }
    else if (l & 1u) 
    {
        return 0;
    }
    const size_t l_2 = l / 2;
    float A0 = ((l_2 & 1u) ? 1.0f : -1.0f) / ((l + 2) * (l - 1));
    float A1 = factorial(l, l_2) / (factorial(l_2) * (1 << l));
    return 2 * F_PI * A0 * A1;
}




void computeShBasis(float*  SHb, size_t numBands, const glm::vec3& s)
{

    /*
     * TODO: all the Legendre computation below is identical for all faces, so it
     * might make sense to pre-compute it once. Also note that there is
     * a fair amount of symmetry within a face (which we could take advantage of
     * to reduce the pre-compute table).
     */

     /*
      * Below, we compute the associated Legendre polynomials using recursion.
      * see: http://mathworld.wolfram.com/AssociatedLegendrePolynomial.html
      *
      * Note [0]: s.z == cos(theta) ==> we only need to compute P(s.z)
      *
      * Note [1]: We in fact compute P(s.z) / sin(theta)^|m|, by removing
      * the "sqrt(1 - s.z*s.z)" [i.e.: sin(theta)] factor from the recursion.
      * This is later corrected in the ( cos(m*phi), sin(m*phi) ) recursion.
      */

      // s = (x, y, z) = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta))

      // handle m=0 separately, since it produces only one coefficient
    float Pml_2 = 0;
    float Pml_1 = 1;
    SHb[0] = Pml_1;
    for (size_t l = 1; l < numBands; l++) 
    {
        float Pml = ((2 * l - 1.0f) * Pml_1 * s.z - (l - 1.0f) * Pml_2) / l;
        Pml_2 = Pml_1;
        Pml_1 = Pml;
        SHb[SHindex(0, l)] = Pml;
    }
    float Pmm = 1;
    for (ssize_t m = 1; m < numBands; m++) 
    {
        Pmm = (1.0f - 2 * m) * Pmm;      // See [1], divide by sqrt(1 - s.z*s.z);
        Pml_2 = Pmm;
        Pml_1 = (2 * m + 1.0f) * Pmm * s.z;
        // l == m
        SHb[SHindex(-m, m)] = Pml_2;
        SHb[SHindex(m, m)] = Pml_2;
        if (m + 1 < numBands) 
        {
            // l == m+1
            SHb[SHindex(-m, m + 1)] = Pml_1;
            SHb[SHindex(m, m + 1)] = Pml_1;
            for (size_t l = m + 2; l < numBands; l++) 
            {
                float Pml = ((2 * l - 1.0f) * Pml_1 * s.z - (l + m - 1.0f) * Pml_2) / (l - m);
                Pml_2 = Pml_1;
                Pml_1 = Pml;
                SHb[SHindex(-m, l)] = Pml;
                SHb[SHindex(m, l)] = Pml;
            }
        }
    }

    // At this point, SHb contains the associated Legendre polynomials divided
    // by sin(theta)^|m|. Below we compute the SH basis.
    //
    // ( cos(m*phi), sin(m*phi) ) recursion:
    // cos(m*phi + phi) == cos(m*phi)*cos(phi) - sin(m*phi)*sin(phi)
    // sin(m*phi + phi) == sin(m*phi)*cos(phi) + cos(m*phi)*sin(phi)
    // cos[m+1] == cos[m]*s.x - sin[m]*s.y
    // sin[m+1] == sin[m]*s.x + cos[m]*s.y
    //
    // Note that (d.x, d.y) == (cos(phi), sin(phi)) * sin(theta), so the
    // code below actually evaluates:
    //      (cos((m*phi), sin(m*phi)) * sin(theta)^|m|
    float Cm = s.x;
    float Sm = s.y;
    for (ssize_t m = 1; m <= numBands; m++) 
    {
        for (size_t l = m; l < numBands; l++) 
        {
            SHb[SHindex(-m, l)] *= Sm;
            SHb[SHindex(m, l)] *= Cm;
        }
        float Cm1 = Cm * s.x - Sm * s.y;
        float Sm1 = Sm * s.x + Cm * s.y;
        Cm = Cm1;
        Sm = Sm1;
    }
}




std::vector<float> getBasis(const glm::vec3& pos, int m_Degree)
{
    float PI = 3.1415926;
    std::vector<float> Y(m_Degree* m_Degree);
    glm::vec3 normal = glm::normalize(pos);
    float x = normal.x;
    float y = normal.y;
    float z = normal.z;

    if (m_Degree >= 1)
    {
        Y[0] = 1.f / 2.f * sqrt(1.f / PI);
    }
    if (m_Degree >= 2)
    {
        Y[1] = -1.0f/2.0f* sqrt(3.f / (PI)) * y;
        Y[2] = 1.0f / 2.0f * sqrt(3.f / (PI)) * z;
        Y[3] = -1.0f / 2.0f * sqrt(3.f / (PI)) * x;
    }
    if (m_Degree >= 3)
    {
        Y[4] = 1.f / 2.f * sqrt(15.f / PI) * x * y;
        Y[5] = -1.f / 2.f * sqrt(15.f / PI) * z * y;
        Y[6] = 1.f / 4.f * sqrt(5.f / PI) * (2*z*z-x*x-y*y);
        Y[7] = -1.f / 2.f * sqrt(15.f / PI) * z * x;
        Y[8] = 1.f / 4.f * sqrt(15.f / PI) * (x * x - y * y);
    }
    return Y;
}


const glm::vec2 invAtan = glm::vec2(0.1591, 0.3183);
glm::vec2 SampleSphericalMap(glm::vec3 v)
{
    glm::vec2 uv = glm::vec2(std::atan2f(v.z, v.x), std::asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}


glm::vec3 ReadHDRPixels(float* imageData, int width, int height, glm::vec2 uv)
{
    uv.x *= width;
    uv.y *= height;
    glm::vec3 pixelData(0, 0, 0);
    //float s = imageData[width * (height - 1) * 3 + (width - 1) * 3];

    pixelData[0] = imageData[int(uv.y * width + uv.x) * 3];
    pixelData[1] = imageData[int(uv.y * width + uv.x) * 3 + 1];
    pixelData[2] = imageData[int(uv.y * width + uv.x) * 3 + 2];
    return pixelData;

    

}


void preprocessSHForShader(std::vector<glm::vec3>& SH) 
{
    constexpr size_t numBands = 3;
    constexpr size_t numCoefs = numBands * numBands;

    constexpr float M_SQRT_PI = 1.7724538509f;
    constexpr float M_SQRT_3 = 1.7320508076f;
    constexpr float M_SQRT_5 = 2.2360679775f;
    constexpr float M_SQRT_15 = 3.8729833462f;
    constexpr float A[numCoefs] = 
    {
                  1.0f / (2.0f * M_SQRT_PI),    // 0  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1 -1
             M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),    // 1  1
             M_SQRT_15 / (2.0f * M_SQRT_PI),    // 2 -2
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3 -1
             M_SQRT_5 / (4.0f * M_SQRT_PI),    // 3  0
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3  1
             M_SQRT_15 / (4.0f * M_SQRT_PI)     // 3  2
    };

    for (size_t i = 0; i < numCoefs; i++) 
    {
        SH[i] *= A[i] * glm::F_1_PI;
    }
}

inline glm::vec2 hammersley(uint32_t i, float iN) 
{
    constexpr float tof = 0.5f / 0x80000000U;
    uint32_t bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return { i * iN, bits * tof };
}

void equirectangularToCubemap(float* imageData, int width, int height)
{

    //const size_t width = width;
    //const size_t height = height;
    //auto toRectilinear = [width, height](glm::vec3 s) -> glm::vec2 {
    //    float xf = std::atan2(s.x, s.z) * glm::F_1_PI;   // range [-1.0, 1.0]
    //    float yf = std::asin(s.y) * (2 * glm::F_1_PI);   // range [-1.0, 1.0]
    //    xf = (xf + 1.0f) * 0.5f * (width - 1);        // range [0, width [
    //    yf = (1.0f - yf) * 0.5f * (height - 1);        // range [0, height[
    //    return glm::vec2(xf, yf);
    //};
    //
    //for(int f = 0; f < 6; f ++)
    //{
    //    for (int y = 0; y < dim; y++)
    //    {
    //        for (size_t x = 0; x < dim; ++x, ++data) 
    //        {
    //            // calculate how many samples we need based on dx, dy in the source
    //            // x = cos(phi) sin(theta)
    //            // y = sin(phi)
    //            // z = cos(phi) cos(theta)
    //
    //            // here we try to figure out how many samples we need, by evaluating the surface
    //            // (in pixels) in the equirectangular -- we take the bounding box of the
    //            // projection of the cubemap texel's corners.
    //
    //            auto pos0 = toRectilinear(getDirectionFor(f, x + 0.0f, y + 0.0f)); // make sure to use the float version
    //            auto pos1 = toRectilinear(getDirectionFor(f, x + 1.0f, y + 0.0f)); // make sure to use the float version
    //            auto pos2 = toRectilinear(getDirectionFor(f, x + 0.0f, y + 1.0f)); // make sure to use the float version
    //            auto pos3 = toRectilinear(getDirectionFor(f, x + 1.0f, y + 1.0f)); // make sure to use the float version
    //            const float minx = std::min(pos0.x, std::min(pos1.x, std::min(pos2.x, pos3.x)));
    //            const float maxx = std::max(pos0.x, std::max(pos1.x, std::max(pos2.x, pos3.x)));
    //            const float miny = std::min(pos0.y, std::min(pos1.y, std::min(pos2.y, pos3.y)));
    //            const float maxy = std::max(pos0.y, std::max(pos1.y, std::max(pos2.y, pos3.y)));
    //            const float dx = std::max(1.0f, maxx - minx);
    //            const float dy = std::max(1.0f, maxy - miny);
    //            const size_t numSamples = size_t(dx * dy);
    //
    //            const float iNumSamples = 1.0f / numSamples;
    //            glm::vec3 c = glm::vec3(0);
    //            for (size_t sample = 0; sample < numSamples; sample++) 
    //            {
    //                // Generate numSamples in our destination pixels and map them to input pixels
    //                const glm::vec2 h = hammersley(uint32_t(sample), iNumSamples);
    //                const glm::vec3 s(getDirectionFor(f, x + h.x, y + h.y));
    //                auto pos = toRectilinear(s);
    //
    //                // we can't use filterAt() here because it reads past the width/height
    //                // which is okay for cubmaps but not for square images
    //
    //                // TODO: the sample should be weighed by the area it covers in the cubemap texel
    //
    //                c += Cubemap::sampleAt(src.getPixelRef((uint32_t)pos.x, (uint32_t)pos.y));
    //            }
    //            c *= iNumSamples;
    //
    //            Cubemap::writeAt(data, c);
    //        }
    //    }
    //}

}




std::vector<glm::vec3> computeSH(size_t numBands, bool irradiance, float *imageData, int width, int height)
{ 
    const size_t numCoefs = numBands * numBands;
    std::vector<glm::vec3> SH(numCoefs);
    float s = imageData[width * (height-1) * 3 + (width-1)*3 ];
    const GLenum faces[6] = 
    {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
      
    };

    auto toRectilinear = [width, height](glm::vec3 s) -> glm::vec2
    {
        float xf = std::atan2(s.x, s.z) * glm::F_1_PI;   // range [-1.0, 1.0]
        float yf = std::asin(s.y) * (2 * glm::F_1_PI);   // range [-1.0, 1.0]
        xf = (xf + 1.0f) * 0.5f * (width - 1);        // range [0, width [
        yf = (1.0f - yf) * 0.5f * (height - 1);        // range [0, height[
        return glm::vec2(xf, yf);
    };

    float rgb[3] = { 0,0,0 };
    int dim = 256;
    for (int f = 0; f < 6; f++)
    {       
        for (size_t y = 0; y <  dim; y++) 
        {
            for (size_t x = 0; x < dim; ++x)
            {

                auto pos0 = toRectilinear(getDirectionFor(faces[f], x + 0.0f, y + 0.0f)); // make sure to use the float version
                auto pos1 = toRectilinear(getDirectionFor(faces[f], x + 1.0f, y + 0.0f)); // make sure to use the float version
                auto pos2 = toRectilinear(getDirectionFor(faces[f], x + 0.0f, y + 1.0f)); // make sure to use the float version
                auto pos3 = toRectilinear(getDirectionFor(faces[f], x + 1.0f, y + 1.0f)); // make sure to use the float version
                const float minx = std::min(pos0.x, std::min(pos1.x, std::min(pos2.x, pos3.x)));
                const float maxx = std::max(pos0.x, std::max(pos1.x, std::max(pos2.x, pos3.x)));
                const float miny = std::min(pos0.y, std::min(pos1.y, std::min(pos2.y, pos3.y)));
                const float maxy = std::max(pos0.y, std::max(pos1.y, std::max(pos2.y, pos3.y)));
                const float dx = std::max(1.0f, maxx - minx);
                const float dy = std::max(1.0f, maxy - miny);
                const size_t numSamples = size_t(dx * dy);

                const float iNumSamples = 1.0f / numSamples;
                glm::vec3 color = glm::vec3(0);
                for (size_t sample = 0; sample < numSamples; sample++)
                {
                    // Generate numSamples in our destination pixels and map them to input pixels
                    const glm::vec2 h = hammersley(uint32_t(sample), iNumSamples);
                    const glm::vec3 s(getDirectionFor(faces[f], x + h.x, y + h.y));
                    auto pos = toRectilinear(s);

                    // we can't use filterAt() here because it reads past the width/height
                    // which is okay for cubmaps but not for square images

                    // TODO: the sample should be weighed by the area it covers in the cubemap texel

                    //c += Cubemap::sampleAt(src.getPixelRef((uint32_t)pos.x, (uint32_t)pos.y));

                    glm::vec3 pixelData(0, 0, 0);
                    //float s = imageData[width * (height - 1) * 3 + (width - 1) * 3];

                    pixelData[0] = imageData[int((uint32_t)pos.y * width + (uint32_t)pos.x) * 3];
                    pixelData[1] = imageData[int((uint32_t)pos.y * width + (uint32_t)pos.x) * 3 + 1];
                    pixelData[2] = imageData[int((uint32_t)pos.y * width + (uint32_t)pos.x) * 3 + 2];
                    color += pixelData;
                }
                color *= iNumSamples;
                //glm::vec3 s = getDirectionFor(faces[f], x+0.5f, y+0.5f);
                //s = glm::normalize(s);
                //glm::vec2 samplePoint = SampleSphericalMap(s);
                //
                //glm::vec3 color = ReadHDRPixels(imageData, width, height, samplePoint);
                // sample a color
                             
                // take solid angle into account
                color *= solidAngle(dim, x, y);
                glm::vec3 s = getDirectionFor(faces[f], x + 0.0f, y + 0.0f);
                std::vector<float> Y = getBasis(s, numBands);
                //computeShBasis(states[f].SHb.get(), numBands, s);
                // apply coefficients to the sampled color
                for (size_t i = 0; i < numCoefs; i++) 
                {
                    SH[i] += color * Y[i];
                }
            }
        }            
    }


    // precompute the scaling factor K
    std::vector<float> K(9, 1);
    //std::vector<float> K = Ki(numBands);
    //K[0] = 0.88623;
    //K[1] = K[2] = K[3] = 1.02333 ;
    //K[4] = K[5] = K[6] = K[7] = K[8] = 0.49542;
    // apply truncated cos (irradiance)
    if (irradiance) 
    {
        for (size_t l = 0; l < numBands; l++) 
        {
            const float truncatedCosSh = computeTruncatedCosSh(size_t(l));
            K[SHindex(0, l)] *= truncatedCosSh;
            for (ssize_t m = 1; m <= l; m++) {
                K[SHindex(-m, l)] *= truncatedCosSh;
                K[SHindex(m, l)] *= truncatedCosSh;
            }
        }
        // apply all the scale factors
        for (size_t i = 0; i < numCoefs; i++)
        {
            SH[i] *= (K[i]);
        }
    }
   
    preprocessSHForShader(SH);


    return SH;
}



void DfgIBLFilter()
{
    glDisable(GL_DEPTH_TEST);
    auto brdfLUTTexture = std::make_shared<ElayGraphics::STexture>();
    brdfLUTTexture->TextureType = ElayGraphics::STexture::ETextureType::Texture2D;
    brdfLUTTexture->InternalFormat = GL_RGB16F;
    brdfLUTTexture->ExternalFormat = GL_RGB;
    brdfLUTTexture->Width = 512;
    brdfLUTTexture->Height = 512;
    brdfLUTTexture->DataType = GL_FLOAT;


    brdfLUTTexture->Type4WrapS = GL_CLAMP_TO_EDGE;
    brdfLUTTexture->Type4WrapT = GL_CLAMP_TO_EDGE;
    brdfLUTTexture->Type4MagFilter = GL_LINEAR;
    brdfLUTTexture->Type4MinFilter = GL_LINEAR;

    genTexture(brdfLUTTexture);
    ElayGraphics::ResourceManager::registerSharedData("brdfLUTTexture", brdfLUTTexture);


    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture->TextureID, 0);

    auto dfgIBLShader = std::make_shared<CShader>("DfgIBL_VS.glsl", "DfgIBL_FS.glsl");
    glViewport(0, 0, 512, 512);
    dfgIBLShader->activeShader();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glEnable(GL_DEPTH_TEST);
}



void SpecularIBLFilter()
{

    glDisable(GL_DEPTH_TEST);
    auto kernelMap = std::make_shared<ElayGraphics::STexture>();
    kernelMap->TextureType = ElayGraphics::STexture::ETextureType::Texture2D;
    kernelMap->InternalFormat = GL_RGBA16F;
    kernelMap->ExternalFormat = GL_RGBA;

    kernelMap->Width = 5;
    kernelMap->Height = 1024;

    kernelMap->DataType = GL_FLOAT;
    kernelMap->Type4WrapS = GL_CLAMP_TO_EDGE;
    kernelMap->Type4WrapT = GL_CLAMP_TO_EDGE;
    kernelMap->Type4WrapR = GL_CLAMP_TO_EDGE;

    kernelMap->Type4MagFilter = GL_NEAREST;
    kernelMap->Type4MinFilter = GL_NEAREST;
    genTexture(kernelMap);


    GLint kernelFBO = genFBO({ kernelMap });
    //genGenerateMipmap(kernelMap);
    ElayGraphics::ResourceManager::registerSharedData("kernelMap", kernelMap);

    auto lodToPerceptualRoughness = [](const float lod) -> float {
        // Inverse perceptualRoughness-to-LOD mapping:
        // The LOD-to-perceptualRoughness mapping is a quadratic fit for
        // log2(perceptualRoughness)+iblMaxMipLevel when iblMaxMipLevel is 4.
        // We found empirically that this mapping works very well for a 256 cubemap with 5 levels used,
        // but also scales well for other iblMaxMipLevel values.
        const float a = 2.0f;
        const float b = -1.0f;
        return (lod != 0.0f) ? glm::saturate((sqrt(a * a + 4.0f * b * lod) - a) / (2.0f * b)) : 0.0f;
    };

    const GLenum faces[2][3] = {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z }
    };

    int mLevelCount = 5;
    float roughnessArray[16] = {};
    for (size_t i = 0, c = mLevelCount; i < c; i++)
    {
        float const perceptualRoughness = lodToPerceptualRoughness(
            glm::saturate(float(i) * (1.0f / (float(mLevelCount) - 1.0f))));
        float const roughness = perceptualRoughness * perceptualRoughness;
        roughnessArray[i] = roughness;
    }

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    //int id = kernelMap->TextureID;
    glBindFramebuffer(GL_FRAMEBUFFER, kernelFBO);
    auto kernelFilterShader = std::make_shared<CShader>("KernelFilter_VS.glsl", "KernelFilter_FS.glsl");
    kernelFilterShader->activeShader();
    for (int i = 0; i < 16; i++)
    {
        std::string setName = "input_roughness[" + std::to_string(i) + "]";
        kernelFilterShader->setFloatUniformValue(setName.c_str(), roughnessArray[i]);
    }
    //kernelFilterShader->setFloatArrayUniformValue("input_roughness", 16, roughnessArray);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 5, 1024);
    drawQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);


    auto prefilterMap = std::make_shared<ElayGraphics::STexture>();
    prefilterMap->TextureType = ElayGraphics::STexture::ETextureType::TextureCubeMap;
    prefilterMap->InternalFormat = GL_RGB16F;
    prefilterMap->ExternalFormat = GL_RGB;
    prefilterMap->Width = 256;
    prefilterMap->Height = 256;
    prefilterMap->DataType = GL_FLOAT;

    prefilterMap->Type4WrapS = GL_CLAMP_TO_EDGE;
    prefilterMap->Type4WrapT = GL_CLAMP_TO_EDGE;
    prefilterMap->Type4WrapR = GL_CLAMP_TO_EDGE;
    prefilterMap->Type4MagFilter = GL_LINEAR;
    prefilterMap->Type4MinFilter = GL_LINEAR_MIPMAP_LINEAR;
    genTexture(prefilterMap);

    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap->TextureID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 4);
    genGenerateMipmap(prefilterMap);

    ElayGraphics::ResourceManager::registerSharedData("prefilterMap", prefilterMap);
    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------

    auto specularFilterShader = std::make_shared<CShader>("SpecularFilter_VS.glsl", "SpecularFilter_FS.glsl");
    specularFilterShader->activeShader();
    //prefilterShader->setIntUniformValue("environmentMap", 0);
    //specularFilterShader->setMat4UniformValue("projection", glm::value_ptr(captureProjection));

    auto envCubemap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("envCubemap");


    specularFilterShader->setTextureUniformValue("materialParams_environment", envCubemap);
    specularFilterShader->setTextureUniformValue("materialParams_kernel", kernelMap);
    float hdrLinear = 1024.0f;   //!< no HDR compression up to this value
    float hdrMax = 16384.0f;     //!< HDR compression between hdrLinear and hdrMax
    float lodOffset = 1.0f;      //!< Good values are 1.0 or 2.0. Higher values help with heavily HDR inputs.

    const float linear = hdrLinear;
    const float compress = hdrMax;
    uint8_t levels = 5;

    uint32_t dim = 256;
    const float omegaP = (4.0f * glm::PI) / float(6 * dim * dim);

    specularFilterShader->setFloatUniformValue("frame_compress", linear, compress);
    specularFilterShader->setFloatUniformValue("frame_lodOffset", lodOffset - log4(omegaP));

    unsigned int sampleCount = 1024;
    for (size_t lod = 0; lod < levels; lod++)
    {

        specularFilterShader->setFloatUniformValue("frame_compress", linear, compress);
        specularFilterShader->setFloatUniformValue("frame_lodOffset", lodOffset - log4(omegaP));
        specularFilterShader->setIntUniformValue("frame_sampleCount", lod == 0 ? 1 : sampleCount);
        specularFilterShader->setIntUniformValue("frame_attachmentLevel", lod);
        float _load_offset = lodOffset - log4(omegaP);
        if (lod == levels - 1)
        {
            // this is the last lod, use a more aggressive filtering because this level is also
            // used for the diffuse brdf by filament, and we need it to be very smooth.
            // So we set the lod offset to at least 2.
            specularFilterShader->setFloatUniformValue("frame_lodOffset", std::max(2.0f, lodOffset) - log4(omegaP));
        }

        glViewport(0, 0, dim, dim);
        for (size_t i = 0; i < 2; i++)
        {
            specularFilterShader->setFloatUniformValue("frame_side", i == 0 ? 1.0f : -1.0f);
            GLuint FBO;
            glGenFramebuffers(1, &FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, faces[i][0], prefilterMap->TextureID, (int)lod);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, faces[i][1], prefilterMap->TextureID, (int)lod);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, faces[i][2], prefilterMap->TextureID, (int)lod);

            int RenderBufferWidth = dim;
            int RenderBufferHeight = dim;
            auto genRenderBufferFunc = [=](GLenum vInternelFormat, GLenum vAttachmentType)
            {
                GLint RenderBuffer;
                glGenRenderbuffers(1, &(GLuint&)RenderBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, RenderBuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, vInternelFormat, RenderBufferWidth, RenderBufferHeight);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, vAttachmentType, GL_RENDERBUFFER, RenderBuffer);
            };

            {
                genRenderBufferFunc(GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
            }
            //{
            //    genRenderBufferFunc(GL_STENCIL_INDEX, GL_STENCIL_ATTACHMENT);
            //}
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "Error::FBO:: Framebuffer Is Not Complete." << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            }
            std::vector<GLenum> Attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 ,GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(static_cast<int>(Attachments.size()), &Attachments[0]);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawQuad();
            glDeleteFramebuffers(1, &FBO);
        }
        glFlush();
        dim >>= 1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}




void IrradianceIBLFilter()
{

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };


    glEnable(GL_DEPTH_TEST);
    auto irradianceMap = std::make_shared<ElayGraphics::STexture>();
    irradianceMap->TextureType = ElayGraphics::STexture::ETextureType::TextureCubeMap;
    irradianceMap->InternalFormat = GL_RGB16F;
    irradianceMap->ExternalFormat = GL_RGB;
    irradianceMap->Width = 256;
    irradianceMap->Height = 256;
    irradianceMap->DataType = GL_FLOAT;


    irradianceMap->Type4WrapS = GL_CLAMP_TO_EDGE;
    irradianceMap->Type4WrapT = GL_CLAMP_TO_EDGE;
    irradianceMap->Type4WrapR = GL_CLAMP_TO_EDGE;
    irradianceMap->Type4MagFilter = GL_LINEAR;
    irradianceMap->Type4MinFilter = GL_LINEAR;

    genTexture(irradianceMap);

    ElayGraphics::ResourceManager::registerSharedData("irradianceMap", irradianceMap);

    auto envCubemap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("envCubemap");

    // -----------------------------------------------------------------------------
    auto irradianceShader = std::make_shared<CShader>("IrradianceConvolution_VS.glsl", "IrradianceConvolution_FS.glsl");
    irradianceShader->activeShader();  
    irradianceShader->setTextureUniformValue("environmentMap", envCubemap);
    
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 256);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    
    glViewport(0, 0, 256, 256); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    int id = irradianceMap->TextureID;
    irradianceShader->setMat4UniformValue("projection", glm::value_ptr(captureProjection));
    for (unsigned int i = 0; i < 6; ++i)
    {    
        irradianceShader->setMat4UniformValue("view", glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->TextureID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawCube();
        glFlush();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

}



void IBLLigthPass::initV()
{

    IBLPath = "../environments/lightroom_14b.hdr";


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_CULL_FACE);

    //stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(IBLPath.c_str(), &width, &height, &nrComponents, 0);
    //unsigned int hdrTexture;
    auto hdrTexture = std::make_shared<ElayGraphics::STexture>();
    if (data)
    {      
        hdrTexture->InternalFormat = GL_RGB16F;
        hdrTexture->ExternalFormat = GL_RGB;
        hdrTexture->Width = width;
        hdrTexture->Height = height;
        hdrTexture->DataType = GL_FLOAT;
        hdrTexture->pDataSet.resize(1);
        hdrTexture->pDataSet[0] = data;

        hdrTexture->Type4WrapS = GL_CLAMP_TO_EDGE;
        hdrTexture->Type4WrapT = GL_CLAMP_TO_EDGE;
        hdrTexture->Type4MagFilter = GL_LINEAR;
        hdrTexture->Type4MinFilter = GL_LINEAR;

        genTexture(hdrTexture);
        ElayGraphics::ResourceManager::registerSharedData("hdrTexture", hdrTexture);

        std::vector<glm::vec3> sh = computeSH(3, true, data, width, height);
        ElayGraphics::ResourceManager::registerSharedData("iblSH", sh);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    auto envCubemap = std::make_shared<ElayGraphics::STexture>();
    envCubemap->TextureType = ElayGraphics::STexture::ETextureType::TextureCubeMap;
    envCubemap->InternalFormat = GL_RGB16F;
    envCubemap->ExternalFormat = GL_RGB;
    envCubemap->Width = 256;
    envCubemap->Height = 256;
    envCubemap->DataType = GL_FLOAT;


    envCubemap->Type4WrapS = GL_CLAMP_TO_EDGE;
    envCubemap->Type4WrapT = GL_CLAMP_TO_EDGE;
    envCubemap->Type4WrapR = GL_CLAMP_TO_EDGE;
    envCubemap->Type4MagFilter = GL_LINEAR;
    envCubemap->Type4MinFilter = GL_LINEAR_MIPMAP_LINEAR;

    //envCubemap->Type4MinFilter = GL_LINEAR;
    genTexture(envCubemap);
    genGenerateMipmap(envCubemap);
    ElayGraphics::ResourceManager::registerSharedData("envCubemap", envCubemap);

 
    auto equirectangularToCubemapShader = std::make_shared<CShader>("EquirectangularToCubeMap_VS.glsl", "EquirectangularToCubeMap_FS.glsl");
    equirectangularToCubemapShader->activeShader();
    equirectangularToCubemapShader->setTextureUniformValue("equirectangularMap", hdrTexture);
    const GLenum faces[2][3] = {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z }
    };
    glViewport(0, 0, 256, 256);
    for (size_t i = 0; i < 2; i++)
    {
        equirectangularToCubemapShader->setFloatUniformValue("frame_side", i == 0 ? 1.0f : -1.0f);
        GLuint FBO;
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, faces[i][0], envCubemap->TextureID, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, faces[i][1], envCubemap->TextureID, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, faces[i][2], envCubemap->TextureID, 0);

        int RenderBufferWidth = 256;
        int RenderBufferHeight = 256;
        auto genRenderBufferFunc = [=](GLenum vInternelFormat, GLenum vAttachmentType)
        {
            GLint RenderBuffer;
            glGenRenderbuffers(1, &(GLuint&)RenderBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, RenderBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, vInternelFormat, RenderBufferWidth, RenderBufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, vAttachmentType, GL_RENDERBUFFER, RenderBuffer);
        };

        {
            genRenderBufferFunc(GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
        }
        //{
        //    genRenderBufferFunc(GL_STENCIL_INDEX, GL_STENCIL_ATTACHMENT);
        //}
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error::FBO:: Framebuffer Is Not Complete." << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        }
        std::vector<GLenum> Attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 ,GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(static_cast<int>(Attachments.size()), &Attachments[0]);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawQuad();

        glDeleteFramebuffers(1, &FBO);
    }
    genGenerateMipmap(envCubemap);
    glFlush();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    DfgIBLFilter();
    IrradianceIBLFilter();
    SpecularIBLFilter();

    glViewport(0, 0, ElayGraphics::WINDOW_KEYWORD::getWindowWidth(), ElayGraphics::WINDOW_KEYWORD::getWindowHeight());
}

void IBLLigthPass::updateV()
{



}


