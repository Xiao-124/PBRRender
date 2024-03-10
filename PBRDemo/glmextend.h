#pragma once

#include <GLM/glm.hpp>
#include <iostream>
#include <cmath>
#include "half.h"
namespace glm
{

    constexpr const double F_E = 2.71828182845904523536028747135266250;
    constexpr const double F_LOG2E = 1.44269504088896340735992468100189214;
    constexpr const double F_LOG10E = 0.434294481903251827651128918916605082;
    constexpr const double F_LN2 = 0.693147180559945309417232121458176568;
    constexpr const double F_LN10 = 2.30258509299404568401799145468436421;
    constexpr const double F_PI = 3.14159265358979323846264338327950288;
    constexpr const double F_PI_2 = 1.57079632679489661923132169163975144;
    constexpr const double F_PI_4 = 0.785398163397448309615660845819875721;
    constexpr const double F_1_PI = 0.318309886183790671537767526745028724;
    constexpr const double F_2_PI = 0.636619772367581343075535053490057448;
    constexpr const double F_2_SQRTPI = 1.12837916709551257389615890312154517;
    constexpr const double F_SQRT2 = 1.41421356237309504880168872420969808;
    constexpr const double F_SQRT1_2 = 0.707106781186547524400844362104849039;
    constexpr const double F_TAU = 2.0 * F_PI;


    constexpr const double E = F_E;
    constexpr const double LOG2E = F_LOG2E;
    constexpr const double LOG10E = F_LOG10E;
    constexpr const double LN2 = F_LN2;
    constexpr const double LN10 = F_LN10;
    constexpr const double PI = F_PI;
    constexpr const double PI_2 = F_PI_2;
    constexpr const double PI_4 = F_PI_4;
    constexpr const double ONE_OVER_PI = F_1_PI;
    constexpr const double TWO_OVER_PI = F_2_PI;
    constexpr const double TWO_OVER_SQRTPI = F_2_SQRTPI;
    constexpr const double SQRT2 = F_SQRT2;
    constexpr const double SQRT1_2 = F_SQRT1_2;
    constexpr const double TAU = F_TAU;
    constexpr const double DEG_TO_RAD = F_PI / 180.0;
    constexpr const double RAD_TO_DEG = 180.0 / F_PI;



    //using namespace ;
    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> cbrt(glm::tvec3<T, P> v)
    {
        for (size_t i = 0; i < 3; i++) {
            v[i] = std::cbrt(v[i]);
        }
        return v;
    }

    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> pow(glm::tvec3<T, P> v, float p)
    {
        for (size_t i = 0; i < 3; i++)
        {
            v[i] = std::pow(v[i], p);
        }
        return v;
    }

    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> pow(T v, glm::tvec3<T, P> p)
    {
        for (size_t i = 0; i < 3; i++) 
        {
            p[i] = std::pow(v, p[i]);
        }
        return p;
    }

    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> pow(glm::tvec3<T, P> v, glm::tvec3<T, P> p) {
        for (size_t i = 0; i < 3; i++) 
        {
            v[i] = std::pow(v[i], p[i]);
        }
        return v;
    }


    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> log10(glm::tvec3<T, P> v) 
    {
        for (size_t i = 0; i < 3; i++) 
        {
            v[i] = std::log10(v[i]);
        }
        return v;
    }

    template <typename T, glm::precision P = defaultp>
    inline constexpr T max(const glm::tvec3<T, P> & v) 
    {
        T r(v[0]);
        for (size_t i = 1; i < 3; i++) 
        {
            r = max(r, v[i]);
        }
        return r;
    }
   
   
   
    template <typename T, glm::precision P = defaultp>
    inline constexpr T min(const glm::tvec3<T, P>& v)
    {
        T r(v[0]);
        for (size_t i = 1; i < 3; i++) 
        {
            r = min(r, v[i]);
        }
        return r;
    }
   //
   // template <typename T, glm::precision P = defaultp>
   // inline constexpr glm::tvec3<T, P> saturate(const glm::tvec3<T, P>& lv)
   // {
   //     return clamp(lv, T(0), T(1));
   // }
   //
   // template <typename T, glm::precision P = defaultp>
   // inline constexpr glm::tvec3<T, P> clamp(glm::tvec3<T, P> v, T min, T max)
   // {
   //     for (size_t i = 0; i < 3; i++) 
   //     {
   //         v[i] = std::min(max, std::max(min, v[i]));
   //     }
   //     return v;
   // }
   //
   // template <typename T, glm::precision P = defaultp>
   // inline constexpr glm::tvec3<T, P> clamp(glm::tvec3<T, P> v, glm::tvec3<T, P> min, glm::tvec3<T, P> max)
   // {
   //     for (size_t i = 0; i < 3; i++) 
   //     {
   //         v[i] = std::min(max[i], std::max(min[i], v[i]));
   //     }
   //     return v;
   // }



    template<typename T>
    inline constexpr T saturate(T v) noexcept 
    {
        return clamp(v, T(0), T(1));
    }

    template <typename T, glm::precision P = defaultp>
    inline glm::tvec2<T, P> xy(glm::tvec3<T, P> v)
    {
        return glm::tvec2<T, P>(v.x, v.y);
    }

    template <typename T, glm::precision P = defaultp>
    inline glm::tvec3<T, P> rgb(glm::tvec4<T, P> v)
    {
        return glm::tvec3<T, P>(v.x, v.y, v.z);
    }


    using half4 = glm::tvec4<math::half>;

}