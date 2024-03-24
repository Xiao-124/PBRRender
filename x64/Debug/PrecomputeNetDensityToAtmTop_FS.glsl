#version 450 core
in vec2  v2f_TexCoords;
out vec2 f2Density;



#define float4 vec4
#define float3 vec3
#define float2 vec2

#define int4 ivec4
#define int3 ivec3
#define int2 ivec2

#define uint4 uvec4
#define uint3 uvec3
#define uint2 uvec2

#define bool4 bvec4
#define bool3 bvec3
#define bool2 bvec2

// OpenGL matrices in GLSL are always as column-major
// (this is not related to how they are stored)
#define float2x2 mat2x2
#define float2x3 mat3x2
#define float2x4 mat4x2

#define float3x2 mat2x3
#define float3x3 mat3x3
#define float3x4 mat4x3

#define float4x2 mat2x4
#define float4x3 mat3x4
#define float4x4 mat4x4
#define matrix mat4x4

#define EARTH_RADIUS 6371000.0
#define DEFAULT_VALUE(x) = x

#define lerp mix

float saturate( float x ){ return clamp( x, 0.0,                      1.0 ); }
vec2  saturate( vec2  x ){ return clamp( x, vec2(0.0, 0.0),           vec2(1.0, 1.0) ); }
vec3  saturate( vec3  x ){ return clamp( x, vec3(0.0, 0.0, 0.0),      vec3(1.0, 1.0, 1.0) ); }
vec4  saturate( vec4  x ){ return clamp( x, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0) ); }

struct AirScatteringAttribs
{
    // Angular Rayleigh scattering coefficient contains all the terms exepting 1 + cos^2(Theta):
    // Pi^2 * (n^2-1)^2 / (2*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4AngularRayleighSctrCoeff;
    // Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions
    // and is the following:
    // 8 * Pi^3 * (n^2-1)^2 / (3*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4TotalRayleighSctrCoeff;
    float4 f4RayleighExtinctionCoeff;

    // Note that angular scattering coefficient is essentially a phase function multiplied by the
    // total scattering coefficient
    float4 f4AngularMieSctrCoeff;
    float4 f4TotalMieSctrCoeff;
    float4 f4MieExtinctionCoeff;

    float4 f4TotalExtinctionCoeff;
    // Cornette-Shanks phase function (see Nishita et al. 93) normalized to unity has the following form:
    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
    float4 f4CS_g; // x == 3*(1-g^2) / (2*(2+g^2))
                   // y == 1 + g^2
                   // z == -2*g

    // Earth parameters can't be changed at run time
    //float fEarthRadius  = 6371000.0f;
    //float fAtmBottomAltitude  = 0.f;     // Altitude of the bottom atmosphere boundary (sea level by default)
    //float fAtmTopAltitude = 80000.f; // Altitude of the top atmosphere boundary, 80 km by default
    //float fTurbidity  = 1.02f;
    //
    //float fAtmBottomRadius    = fEarthRadius + fAtmBottomAltitude;
    //float fAtmTopRadius     = fEarthRadius + fAtmTopAltitude;
    //float fAtmAltitudeRangeInv   = 1.f / (fAtmTopAltitude - fAtmBottomAltitude);
    //float fAerosolPhaseFuncG    = 0.76f;
    //
    //float4 f4ParticleScaleHeight  = float4(7994.f, 1200.f, 1.f/7994.f, 1.f/1200.f;

    float fEarthRadius;
    float fAtmBottomAltitude;
    float fAtmTopAltitude;
    float fTurbidity;
    float fAtmBottomRadius;
    float fAtmTopRadius;
    float fAtmAltitudeRangeInv;
    float fAerosolPhaseFuncG;
    float4 f4ParticleScaleHeight;
};


//uniform cbParticipatingMediaScatteringParams
//{
//    AirScatteringAttribs g_MediaParams;
//};

layout (std430, binding = 0) buffer cbParticipatingMediaScatteringParams
{
	AirScatteringAttribs g_MediaParams;
};


void GetRaySphereIntersection(in  float3 f3RayOrigin,
                              in  float3 f3RayDirection,
                              in  float3 f3SphereCenter,
                              in  float  fSphereRadius,
                              out float2 f2Intersections)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2.0 * dot(f3RayOrigin, f3RayDirection);
    float C = dot(f3RayOrigin,f3RayOrigin) - fSphereRadius*fSphereRadius;
    float D = B*B - 4.0*A*C;
    // If discriminant is negative, there are no real roots hence the ray misses the
    // sphere
    if (D < 0.0)
    {
        f2Intersections = float2(-1.0, -1.0);
    }
    else
    {
        D = sqrt(D);
        f2Intersections = float2(-B - D, -B + D) / (2.0*A); // A must be positive here!!
    }
}

float2 IntegrateParticleDensity(float3 f3Start,
                                float3 f3End,
                                float3 f3EarthCentre,
                                int    iNumSteps )
{
    float3 f3Step = (f3End - f3Start) / float(iNumSteps);
    float fStepLen = length(f3Step);

    float fStartHeightAboveSurface = abs( length(f3Start - f3EarthCentre) - g_MediaParams.fEarthRadius );
    float2 f2PrevParticleDensity = exp( -fStartHeightAboveSurface * g_MediaParams.f4ParticleScaleHeight.zw );

    float2 f2ParticleNetDensity = float2(0.0, 0.0);
    for (int iStepNum = 1; iStepNum <= iNumSteps; ++iStepNum)
    {
        float3 f3CurrPos = f3Start + f3Step * float(iStepNum);
        float fHeightAboveSurface = abs( length(f3CurrPos - f3EarthCentre) - g_MediaParams.fEarthRadius );
        float2 f2ParticleDensity = exp( -fHeightAboveSurface * g_MediaParams.f4ParticleScaleHeight.zw );
        f2ParticleNetDensity += (f2ParticleDensity + f2PrevParticleDensity) * fStepLen / 2.0;
        f2PrevParticleDensity = f2ParticleDensity;
    }
    return f2ParticleNetDensity;
}

float2 IntegrateParticleDensityAlongRay(float3     f3Pos,
                                        float3     f3RayDir,
                                        float3     f3EarthCentre,
                                        const int  iNumSteps,
                                        const bool bOccludeByEarth)
{
    if( bOccludeByEarth )
    {
        // If the ray hits the bottom atmosphere boundary, return huge optical depth
        float2 f2RayEarthIsecs;
        GetRaySphereIntersection(f3Pos, f3RayDir, f3EarthCentre, g_MediaParams.fAtmBottomRadius, f2RayEarthIsecs);
        if( f2RayEarthIsecs.x > 0.0 )
            return float2(1e+20, 1e+20);
    }

    // Get intersection with the top of the atmosphere (the start point must always be under the top of it)
    //
    //                     /
    //                .   /  .
    //      .  '         /\         '  .
    //                  /  f2RayAtmTopIsecs.y > 0
    //                 *
    //                   f2RayAtmTopIsecs.x < 0
    //                  /
    //
    float2 f2RayAtmTopIsecs;
    GetRaySphereIntersection(f3Pos, f3RayDir, f3EarthCentre, g_MediaParams.fAtmTopRadius, f2RayAtmTopIsecs);
    float fIntegrationDist = f2RayAtmTopIsecs.y;

    float3 f3RayEnd = f3Pos + f3RayDir * fIntegrationDist;

    return IntegrateParticleDensity(f3Pos, f3RayEnd, f3EarthCentre, iNumSteps);
}




void main()
{

    float2 f2UV = v2f_TexCoords;
    // Do not allow start point be at the Earth surface and on the top of the atmosphere
    float fStartHeight = clamp( lerp(g_MediaParams.fAtmBottomAltitude, g_MediaParams.fAtmTopAltitude, f2UV.x), 10.0, g_MediaParams.fAtmTopAltitude-10.0 );

    float fCosTheta = f2UV.y * 2.0 - 1.0;
    float fSinTheta = sqrt( saturate(1.0 - fCosTheta*fCosTheta) );
    float3 f3RayStart = float3(0.0, 0.0, fStartHeight);
    float3 f3RayDir = float3(fSinTheta, 0.0, fCosTheta);

    float3 f3EarthCentre = float3(0.0, 0.0, -g_MediaParams.fEarthRadius);
    const int iNumSteps = 200;
    f2Density = IntegrateParticleDensityAlongRay(f3RayStart, f3RayDir, f3EarthCentre, iNumSteps, true);
}