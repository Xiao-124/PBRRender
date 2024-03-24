#version 440 core


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

#define _SWIZZLE0
#define _SWIZZLE1 .x
#define _SWIZZLE2 .xy
#define _SWIZZLE3 .xyz
#define _SWIZZLE4 .xyzw


#define _GET_GL_GLOBAL_INVOCATION_ID(Type, InvocId)InvocId=Type(gl_GlobalInvocationID)
#define THREAD_GROUP_SIZE 16
#define PRECOMPUTED_SCTR_LUT_DIM float4(32.0,128.0,64.0,16.0)

#define NON_LINEAR_PARAMETERIZATION 1
#define SafetyHeightMargin 16.0
#define HeightPower 0.5
#define ViewZenithPower 0.2
#define SunViewPower 1.5
#define PI 3.141592653
#define NUM_RANDOM_SPHERE_SAMPLES 128

#define SamplerState int
#define SampleLevel_3(Tex, Sampler, Coords, Level)         textureLod(Tex, _ToVec(Coords), _ToFloat(Level))
#define IMAGE_WRITEONLY writeonly
#define rsqrt inversesqrt


int _ToInt( int x )    { return int(x);     }
int _ToInt( ivec2 v )  { return int(v.x);   }
int _ToInt( ivec3 v )  { return int(v.x);   }
int _ToInt( ivec4 v )  { return int(v.x);   }

int _ToInt( uint x )   { return int(x);     }
int _ToInt( uvec2 v )  { return int(v.x);   }
int _ToInt( uvec3 v )  { return int(v.x);   }
int _ToInt( uvec4 v )  { return int(v.x);   }

int _ToInt( float x )  { return int(x);     }
int _ToInt( vec2 v )   { return int(v.x);   }
int _ToInt( vec3 v )   { return int(v.x);   }
int _ToInt( vec4 v )   { return int(v.x);   }

int _ToInt( bool x )   { return x   ? 1 : 0;}
int _ToInt( bvec2 v )  { return v.x ? 1 : 0;}
int _ToInt( bvec3 v )  { return v.x ? 1 : 0;}
int _ToInt( bvec4 v )  { return v.x ? 1 : 0;}



float _ToFloat( int x )  { return float(x);     }
float _ToFloat( ivec2 v ){ return float(v.x);   }
float _ToFloat( ivec3 v ){ return float(v.x);   }
float _ToFloat( ivec4 v ){ return float(v.x);   }

float _ToFloat( uint x ) { return float(x);     }
float _ToFloat( uvec2 v ){ return float(v.x);   }
float _ToFloat( uvec3 v ){ return float(v.x);   }
float _ToFloat( uvec4 v ){ return float(v.x);   }

float _ToFloat( float x ){ return float(x);     }
float _ToFloat( vec2 v ) { return float(v.x);   }
float _ToFloat( vec3 v ) { return float(v.x);   }
float _ToFloat( vec4 v ) { return float(v.x);   }

float _ToFloat( bool x ) { return x   ? 1.0 : 0.0;}
float _ToFloat( bvec2 v ){ return v.x ? 1.0 : 0.0;}
float _ToFloat( bvec3 v ){ return v.x ? 1.0 : 0.0;}
float _ToFloat( bvec4 v ){ return v.x ? 1.0 : 0.0;}



uint _ToUint( int x )  { return uint(x);     }
uint _ToUint( uint x ) { return uint(x);     }
uint _ToUint( float x ){ return uint(x);     }
uint _ToUint( bool x ) { return x ? 1u : 0u; }

bool _ToBool( int x )  { return x != 0   ? true : false; }
bool _ToBool( uint x ) { return x != 0u  ? true : false; }
bool _ToBool( float x ){ return x != 0.0 ? true : false; }
bool _ToBool( bool x ) { return x; }

#define _ToVec2(x,y)     vec2(_ToFloat(x), _ToFloat(y))
#define _ToVec3(x,y,z)   vec3(_ToFloat(x), _ToFloat(y), _ToFloat(z))
#define _ToVec4(x,y,z,w) vec4(_ToFloat(x), _ToFloat(y), _ToFloat(z), _ToFloat(w))

#define _ToIvec2(x,y)     ivec2(_ToInt(x), _ToInt(y))
#define _ToIvec3(x,y,z)   ivec3(_ToInt(x), _ToInt(y), _ToInt(z))
#define _ToIvec4(x,y,z,w) ivec4(_ToInt(x), _ToInt(y), _ToInt(z), _ToInt(w))

#define _ToUvec2(x,y)     uvec2(_ToUint(x), _ToUint(y))
#define _ToUvec3(x,y,z)   uvec3(_ToUint(x), _ToUint(y), _ToUint(z))
#define _ToUvec4(x,y,z,w) uvec4(_ToUint(x), _ToUint(y), _ToUint(z), _ToUint(w))

#define _ToBvec2(x,y)     bvec2(_ToBool(x), _ToBool(y))
#define _ToBvec3(x,y,z)   bvec3(_ToBool(x), _ToBool(y), _ToBool(z))
#define _ToBvec4(x,y,z,w) bvec4(_ToBool(x), _ToBool(y), _ToBool(z), _ToBool(w))


int   _ToIvec( uint  u1 ){ return _ToInt(   u1 ); }
ivec2 _ToIvec( uvec2 u2 ){ return _ToIvec2( u2.x, u2.y ); }
ivec3 _ToIvec( uvec3 u3 ){ return _ToIvec3( u3.x, u3.y, u3.z ); }
ivec4 _ToIvec( uvec4 u4 ){ return _ToIvec4( u4.x, u4.y, u4.z, u4.w ); }

int   _ToIvec( int   i1 ){ return i1; }
ivec2 _ToIvec( ivec2 i2 ){ return i2; }
ivec3 _ToIvec( ivec3 i3 ){ return i3; }
ivec4 _ToIvec( ivec4 i4 ){ return i4; }

int   _ToIvec( float f1 ){ return _ToInt(   f1 ); }
ivec2 _ToIvec( vec2  f2 ){ return _ToIvec2( f2.x, f2.y ); }
ivec3 _ToIvec( vec3  f3 ){ return _ToIvec3( f3.x, f3.y, f3.z ); }
ivec4 _ToIvec( vec4  f4 ){ return _ToIvec4( f4.x, f4.y, f4.z, f4.w ); }


float _ToVec( uint  u1 ){ return _ToFloat(u1); }
vec2  _ToVec( uvec2 u2 ){ return _ToVec2( u2.x, u2.y ); }
vec3  _ToVec( uvec3 u3 ){ return _ToVec3( u3.x, u3.y, u3.z ); }
vec4  _ToVec( uvec4 u4 ){ return _ToVec4( u4.x, u4.y, u4.z, u4.w ); }

float _ToVec( int   i1 ){ return _ToFloat(i1); }
vec2  _ToVec( ivec2 i2 ){ return _ToVec2( i2.x, i2.y ); }
vec3  _ToVec( ivec3 i3 ){ return _ToVec3( i3.x, i3.y, i3.z ); }
vec4  _ToVec( ivec4 i4 ){ return _ToVec4( i4.x, i4.y, i4.z, i4.w ); }

float _ToVec( float f1 ){ return f1; }
vec2  _ToVec( vec2  f2 ){ return f2; }
vec3  _ToVec( vec3  f3 ){ return f3; }
vec4  _ToVec( vec4  f4 ){ return f4; }


uint   _ToUvec( uint  u1 ){ return u1; }
uvec2  _ToUvec( uvec2 u2 ){ return u2; }
uvec3  _ToUvec( uvec3 u3 ){ return u3; }
uvec4  _ToUvec( uvec4 u4 ){ return u4; }

uint   _ToUvec( int   i1 ){ return _ToUint(  i1 ); }
uvec2  _ToUvec( ivec2 i2 ){ return _ToUvec2( i2.x, i2.y ); }
uvec3  _ToUvec( ivec3 i3 ){ return _ToUvec3( i3.x, i3.y, i3.z ); }
uvec4  _ToUvec( ivec4 i4 ){ return _ToUvec4( i4.x, i4.y, i4.z, i4.w ); }

uint   _ToUvec( float f1 ){ return _ToUint(  f1 ); }
uvec2  _ToUvec( vec2  f2 ){ return _ToUvec2( f2.x, f2.y ); }
uvec3  _ToUvec( vec3  f3 ){ return _ToUvec3( f3.x, f3.y, f3.z ); }
uvec4  _ToUvec( vec4  f4 ){ return _ToUvec4( f4.x, f4.y, f4.z, f4.w ); }

#define lerp mix
float saturate( float x ){ return clamp( x, 0.0,                      1.0 ); }
vec2  saturate( vec2  x ){ return clamp( x, vec2(0.0, 0.0),           vec2(1.0, 1.0) ); }
vec3  saturate( vec3  x ){ return clamp( x, vec3(0.0, 0.0, 0.0),      vec3(1.0, 1.0, 1.0) ); }
vec4  saturate( vec4  x ){ return clamp( x, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0) ); }


vec4 _ExpandVector( float x ){ return vec4(    x,    x,    x,    x ); }
vec4 _ExpandVector( vec2 f2 ){ return vec4( f2.x, f2.y,  0.0,  0.0 ); }
vec4 _ExpandVector( vec3 f3 ){ return vec4( f3.x, f3.y, f3.z,  0.0 ); }
vec4 _ExpandVector( vec4 f4 ){ return vec4( f4.x, f4.y, f4.z, f4.w ); }

ivec4 _ExpandVector( int    x ){ return ivec4(    x,    x,    x,    x ); }
ivec4 _ExpandVector( ivec2 i2 ){ return ivec4( i2.x, i2.y,    0,    0 ); }
ivec4 _ExpandVector( ivec3 i3 ){ return ivec4( i3.x, i3.y, i3.z,    0 ); }
ivec4 _ExpandVector( ivec4 i4 ){ return ivec4( i4.x, i4.y, i4.z, i4.w ); }

uvec4 _ExpandVector( uint   x ){ return uvec4(    x,    x,    x,    x ); }
uvec4 _ExpandVector( uvec2 u2 ){ return uvec4( u2.x, u2.y,   0u,   0u ); }
uvec4 _ExpandVector( uvec3 u3 ){ return uvec4( u3.x, u3.y, u3.z,   0u ); }
uvec4 _ExpandVector( uvec4 u4 ){ return uvec4( u4.x, u4.y, u4.z, u4.w ); }

bvec4 _ExpandVector( bool   x ){ return bvec4(    x,    x,     x,     x ); }
bvec4 _ExpandVector( bvec2 b2 ){ return bvec4( b2.x, b2.y, false, false ); }
bvec4 _ExpandVector( bvec3 b3 ){ return bvec4( b3.x, b3.y,  b3.z, false ); }
bvec4 _ExpandVector( bvec4 b4 ){ return bvec4( b4.x, b4.y,  b4.z,  b4.w ); }

#define LoadTex1D_1(Tex, Location)        texelFetch      (Tex, _ToInt((Location).x), _ToInt((Location).y))
#define LoadTex1D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToInt((Location).x), _ToInt((Location).y), int(Offset))
#define LoadTex1DArr_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xy), _ToInt((Location).z) )
#define LoadTex1DArr_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xy), _ToInt((Location).z), int(Offset))
#define LoadTex2D_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xy), _ToInt((Location).z))
#define LoadTex2D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xy), _ToInt((Location).z), ivec2( (Offset).xy) )
#define LoadTex2DArr_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xyz), _ToInt((Location).w) )
#define LoadTex2DArr_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xyz), _ToInt((Location).w), ivec2( (Offset).xy))
#define LoadTex3D_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xyz), _ToInt((Location).w))
#define LoadTex3D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xyz), _ToInt((Location).w), ivec3( (Offset).xyz))
#define LoadTex2DMS_2(Tex, Location, Sample)        texelFetch(Tex, _ToIvec( (Location).xy), _ToInt(Sample))
#define LoadTex2DMS_3(Tex, Location, Sample, Offset)texelFetch(Tex, _ToIvec2( (Location).x + (Offset).x, (Location).y + (Offset).y), int(Sample) ) // No texelFetchOffset for texture2DMS
#define LoadTex2DMSArr_2(Tex, Location, Sample)        texelFetch(Tex, _ToIvec( (Location).xyz), _ToInt(Sample))
#define LoadTex2DMSArr_3(Tex, Location, Sample, Offset)texelFetch(Tex, _ToIvec3( (Location).x + (Offset).x, (Location).y + (Offset).y, (Location).z), int(Sample)) // No texelFetchOffset for texture2DMSArray
#define LoadTexBuffer_1(Tex, Location)  texelFetch(Tex, _ToInt(Location))






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


layout (std430, binding = 0) buffer cbParticipatingMediaScatteringParams
{
	AirScatteringAttribs g_MediaParams;
};


float3 ComputeViewDir(in float fCosViewZenithAngle)
{
    return float3(sqrt(saturate(1.0 - fCosViewZenithAngle*fCosViewZenithAngle)), fCosViewZenithAngle, 0.0);
}

float3 ComputeLightDir(in float3 f3ViewDir, in float fCosSunZenithAngle, in float fCosSunViewAngle)
{
    float3 f3DirOnLight;
    f3DirOnLight.x = (f3ViewDir.x > 0.0) ? (fCosSunViewAngle - fCosSunZenithAngle * f3ViewDir.y) / f3ViewDir.x : 0.0;
    f3DirOnLight.y = fCosSunZenithAngle;
    f3DirOnLight.z = sqrt( saturate(1.0 - dot(f3DirOnLight.xy, f3DirOnLight.xy)) );
    // Do not normalize f3DirOnLight! Even if its length is not exactly 1 (which can
    // happen because of fp precision issues), all the dot products will still be as
    // specified, which is essentially important. If we normalize the vector, all the
    // dot products will deviate, resulting in wrong pre-computation.
    // Since fCosSunViewAngle is clamped to allowable range, f3DirOnLight should always
    // be normalized. However, due to some issues on NVidia hardware sometimes
    // it may not be as that (see IMPORTANT NOTE regarding NVIDIA hardware)
    //f3DirOnLight = normalize(f3DirOnLight);
    return f3DirOnLight;
}

float4 LUTCoordsFromThreadID( uint3 ThreadId )
{
    float4 f4Corrds;
    f4Corrds.xy = (float2( ThreadId.xy ) + float2(0.5,0.5) ) / PRECOMPUTED_SCTR_LUT_DIM.xy;

    uint uiW = ThreadId.z % uint(PRECOMPUTED_SCTR_LUT_DIM.z);
    uint uiQ = ThreadId.z / uint(PRECOMPUTED_SCTR_LUT_DIM.z);
    f4Corrds.zw = ( float2(uiW, uiQ) + float2(0.5,0.5) ) / PRECOMPUTED_SCTR_LUT_DIM.zw;
    return f4Corrds;
}

void ApplyPhaseFunctions(inout float3 f3RayleighInscattering,
                         inout float3 f3MieInscattering,
                         in float cosTheta)
{
    f3RayleighInscattering *= g_MediaParams.f4AngularRayleighSctrCoeff.rgb * (1.0 + cosTheta*cosTheta);

    // Apply Cornette-Shanks phase function (see Nishita et al. 93):
    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
    // f4CS_g = ( 3*(1-g^2) / (2*(2+g^2)), 1+g^2, -2g, 1 )
    float fDenom = rsqrt( dot(g_MediaParams.f4CS_g.yz, float2(1.0, cosTheta)) ); // 1 / (1 + g^2 - 2g*cos(theta))^(1/2)
    float fCornettePhaseFunc = g_MediaParams.f4CS_g.x * (fDenom*fDenom*fDenom) * (1.0 + cosTheta*cosTheta);
    f3MieInscattering *= g_MediaParams.f4AngularMieSctrCoeff.rgb * fCornettePhaseFunc;
}

float GetCosHorizonAngle(float fAltitude, float fSphereRadius)
{
    fAltitude = max(fAltitude, 0.0);
    return -sqrt(fAltitude * (2.0 * fSphereRadius + fAltitude)) / (fSphereRadius + fAltitude);
}


float ZenithAngle2TexCoord(float fCosZenithAngle,       // Cosine of the zenith angle
                           float fAltitude,             // Altitude (height above the sea level)
                           float fEarthRadius,          // Earth radius at sea level
                           float fAtmBottomAltitude,    // Altitude of the bottom atmosphere boundary (wrt sea level)
                           float fTexDim,               // Look-up texture dimension
                           float power,                 // Non-linear transform power
                           float fPrevTexCoord          // Previous look-up texture coordinate
                          )
{
    float fCosHorizonAngle = GetCosHorizonAngle(fAltitude - fAtmBottomAltitude, fEarthRadius + fAtmBottomAltitude);

    // When performing look-ups into the scattering texture, it is very important that all the look-ups are consistent
    // wrt to the horizon. This means that if the first look-up is above (below) horizon, then the second look-up
    // should also be above (below) horizon.
    // We use previous texture coordinate, if it is provided, to find out if previous look-up was above or below
    // horizon. If texture coordinate is negative, then this is the first look-up
    bool bIsAboveHorizon = fPrevTexCoord >= 0.5;
    bool bIsBelowHorizon = 0.0 <= fPrevTexCoord && fPrevTexCoord < 0.5;

    float fTexCoord;
    if ( bIsAboveHorizon ||
        !bIsBelowHorizon && (fCosZenithAngle > fCosHorizonAngle))
    {
        // Scale to [0,1]
        fTexCoord = saturate( (fCosZenithAngle - fCosHorizonAngle) / (1.0 - fCosHorizonAngle) );
        fTexCoord = pow(fTexCoord, power);
        // Now remap texture coordinate to the upper half of the texture.
        // To avoid filtering across discontinuity at 0.5, we must map
        // the texture coordinate to [0.5 + 0.5/fTexDim, 1 - 0.5/fTexDim]
        //
        //      0.5   1.5               D/2+0.5        D-0.5  texture coordinate x dimension
        //       |     |                   |            |
        //    |  X  |  X  | .... |  X  ||  X  | .... |  X  |
        //       0     1          D/2-1   D/2          D-1    texel index
        //
        fTexCoord = 0.5 + 0.5 / fTexDim + fTexCoord * (fTexDim/2.0 - 1.0) / fTexDim;
    }
    else
    {
        fTexCoord = saturate( (fCosHorizonAngle - fCosZenithAngle) / (fCosHorizonAngle - (-1.0)) );
        fTexCoord = pow(fTexCoord, power);
        // Now remap texture coordinate to the lower half of the texture.
        // To avoid filtering across discontinuity at 0.5, we must map
        // the texture coordinate to [0.5/fTexDim, 0.5 - 0.5/fTexDim]
        //
        //      0.5   1.5        D/2-0.5             texture coordinate x dimension
        //       |     |            |
        //    |  X  |  X  | .... |  X  ||  X  | ....
        //       0     1          D/2-1   D/2        texel index
        //
        fTexCoord = 0.5 / fTexDim + fTexCoord * (fTexDim/2.0 - 1.0) / fTexDim;
    }

    return fTexCoord;
}


float TexCoord2ZenithAngle(float fTexCoord,             // Texture coordinate
                           float fAltitde,              // Altitude (height above the sea level)
                           float fEarthRadius,          // Earth radius at sea level
                           float fAtmBottomAltitude,    // Altitude of the bottom atmosphere boundary (wrt sea level)
                           float fTexDim,               // Look-up texture dimension
                           float power                  // Non-linear transform power
                          )
{
    float fCosZenithAngle;

    float fCosHorzAngle = GetCosHorizonAngle(fAltitde - fAtmBottomAltitude, fEarthRadius + fAtmBottomAltitude);
    if( fTexCoord > 0.5 )
    {
        // Remap to [0,1] from the upper half of the texture [0.5 + 0.5/fTexDim, 1 - 0.5/fTexDim]
        fTexCoord = saturate( (fTexCoord - (0.5 + 0.5 / fTexDim)) * fTexDim / (fTexDim/2.0 - 1.0) );
        fTexCoord = pow(fTexCoord, 1.0/power);
        // Assure that the ray does NOT hit Earth
        fCosZenithAngle = max( (fCosHorzAngle + fTexCoord * (1.0 - fCosHorzAngle)), fCosHorzAngle + 1e-4);
    }
    else
    {
        // Remap to [0,1] from the lower half of the texture [0.5/fTexDim, 0.5 - 0.5/fTexDim]
        fTexCoord = saturate((fTexCoord - 0.5 / fTexDim) * fTexDim / (fTexDim/2.0 - 1.0));
        fTexCoord = pow(fTexCoord, 1.0/power);
        // Assure that the ray DOES hit Earth
        fCosZenithAngle = min( (fCosHorzAngle - fTexCoord * (fCosHorzAngle - (-1.0))), fCosHorzAngle - 1e-4);
    }
    return fCosZenithAngle;
}





// Transforms inscattering look-up table coordinates into world parameters
void InsctrLUTCoords2WorldParams(in  float4 f4UVWQ,
                                 in  float  fEarthRadius,
                                 in  float  fAtmBottomAltitude,
                                 in  float  fAtmTopAltitude,
                                 out float  fAltitude,
                                 out float  fCosViewZenithAngle,
                                 out float  fCosSunZenithAngle,
                                 out float  fCosSunViewAngle)
{
#if NON_LINEAR_PARAMETERIZATION
    // Rescale to exactly 0,1 range
    f4UVWQ.xzw = saturate(( f4UVWQ * PRECOMPUTED_SCTR_LUT_DIM - float4(0.5,0.5,0.5,0.5) ) / ( PRECOMPUTED_SCTR_LUT_DIM - float4(1.0,1.0,1.0,1.0) )).xzw;

    f4UVWQ.x = pow( f4UVWQ.x, 1.0/HeightPower );
    // Allowable altitude range is limited to [fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries
    fAltitude = f4UVWQ.x * ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0 * SafetyHeightMargin) + (fAtmBottomAltitude + SafetyHeightMargin);

    fCosViewZenithAngle = TexCoord2ZenithAngle(f4UVWQ.y, fAltitude, fEarthRadius, fAtmBottomAltitude, PRECOMPUTED_SCTR_LUT_DIM.y, ViewZenithPower);

    // Use Eric Bruneton's formula for cosine of the sun-zenith angle
    fCosSunZenithAngle = tan((2.0 * f4UVWQ.z - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);

    f4UVWQ.w = sign(f4UVWQ.w - 0.5) * pow( abs((f4UVWQ.w - 0.5)*2.0), 1.0/SunViewPower)/2.0 + 0.5;
    fCosSunViewAngle = cos(f4UVWQ.w*PI);
#else
    // Rescale to exactly 0,1 range
    f4UVWQ = (f4UVWQ * PRECOMPUTED_SCTR_LUT_DIM - float4(0.5,0.5,0.5,0.5)) / (PRECOMPUTED_SCTR_LUT_DIM-float4(1.0,1.0,1.0,1.0));

    // Allowable altitude range is limited to [fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries
    fAltitude = f4UVWQ.x * ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0 * SafetyHeightMargin) + (fAtmBottomAltitude + SafetyHeightMargin);

    fCosViewZenithAngle = f4UVWQ.y * 2.0 - 1.0;
    fCosSunZenithAngle  = f4UVWQ.z * 2.0 - 1.0;
    fCosSunViewAngle    = f4UVWQ.w * 2.0 - 1.0;
#endif

    fCosViewZenithAngle = clamp(fCosViewZenithAngle, -1.0, +1.0);
    fCosSunZenithAngle  = clamp(fCosSunZenithAngle,  -1.0, +1.0);
    // Compute allowable range for the cosine of the sun view angle for the given
    // view zenith and sun zenith angles
    float D = (1.0 - fCosViewZenithAngle * fCosViewZenithAngle) * (1.0 - fCosSunZenithAngle  * fCosSunZenithAngle);

    // !!!!  IMPORTANT NOTE regarding NVIDIA hardware !!!!

    // There is a very weird issue on NVIDIA hardware with clamp(), saturate() and min()/max()
    // functions. No matter what function is used, fCosViewZenithAngle and fCosSunZenithAngle
    // can slightly fall outside [-1,+1] range causing D to be negative
    // Using saturate(D), max(D, 0) and even D>0?D:0 does not work!
    // The only way to avoid taking the square root of negative value and obtaining NaN is
    // to use max() with small positive value:
    D = sqrt( max(D, 1e-20) );

    // The issue was reproduceable on NV GTX 680, driver version 9.18.13.2723 (9/12/2013).
    // The problem does not arise on Intel hardware

    float2 f2MinMaxCosSunViewAngle = fCosViewZenithAngle*fCosSunZenithAngle + float2(-D, +D);
    // Clamp to allowable range
    fCosSunViewAngle    = clamp(fCosSunViewAngle, f2MinMaxCosSunViewAngle.x, f2MinMaxCosSunViewAngle.y);
}


float4 WorldParams2InsctrLUTCoords(float  fAltitude,
                                   float  fCosViewZenithAngle,
                                   float  fCosSunZenithAngle,
                                   float  fCosSunViewAngle,
                                   float  fEarthRadius,
                                   float  fAtmBottomAltitude,
                                   float  fAtmTopAltitude,
                                   float4 f4RefUVWQ)
{
    float4 f4UVWQ;

    // Limit allowable altitude range to [fAtmBottomAltitude + SafetyHeightMargin, AtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries.
    // (ray/sphere intersection tests are unstable when fAltitude == fAtmBottomAltitude and fAltitude == AtmTopAltitude)
    fAltitude = clamp(fAltitude, fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin);
    f4UVWQ.x = saturate( (fAltitude - (fAtmBottomAltitude + SafetyHeightMargin)) / ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0*SafetyHeightMargin) );

#if NON_LINEAR_PARAMETERIZATION
    f4UVWQ.x = pow(f4UVWQ.x, HeightPower);

    f4UVWQ.y = ZenithAngle2TexCoord(fCosViewZenithAngle, fAltitude, fEarthRadius, fAtmBottomAltitude, PRECOMPUTED_SCTR_LUT_DIM.y, ViewZenithPower, f4RefUVWQ.y);

    // Use Eric Bruneton's formula for cosine of the sun-zenith angle
    f4UVWQ.z = (atan(max(fCosSunZenithAngle, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5;

    fCosSunViewAngle = clamp(fCosSunViewAngle, -1.0, +1.0);
    f4UVWQ.w = acos(fCosSunViewAngle) / PI;
    f4UVWQ.w = sign(f4UVWQ.w - 0.5) * pow( abs((f4UVWQ.w - 0.5)/0.5), SunViewPower)/2.0 + 0.5;

    f4UVWQ.xzw = ((f4UVWQ * (PRECOMPUTED_SCTR_LUT_DIM - float4(1.0, 1.0, 1.0, 1.0)) + float4(0.5, 0.5, 0.5, 0.5)) / PRECOMPUTED_SCTR_LUT_DIM).xzw;
#else
    f4UVWQ.y = (fCosViewZenithAngle+1.) / 2.;
    f4UVWQ.z = (fCosSunZenithAngle +1.) / 2.;
    f4UVWQ.w = (fCosSunViewAngle   +1.) / 2.;

    f4UVWQ = (f4UVWQ * (PRECOMPUTED_SCTR_LUT_DIM - float4(1.0,1.0,1.0,1.0)) + float4(0.5,0.5,0.5,0.5)) / PRECOMPUTED_SCTR_LUT_DIM;
#endif

    return f4UVWQ;
}

void GetRaySphereIntersection2(in  float3 f3RayOrigin,
                               in  float3 f3RayDirection,
                               in  float3 f3SphereCenter,
                               in  float2 f2SphereRadius,
                               out float4 f4Intersections)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2.0 * dot(f3RayOrigin, f3RayDirection);
    float2 C = dot(f3RayOrigin,f3RayOrigin) - f2SphereRadius*f2SphereRadius;
    float2 D = B*B - 4.0*A*C;
    // If discriminant is negative, there are no real roots hence the ray misses the
    // sphere
    float2 f2RealRootMask = float2(D.x >= 0.0 ? 1.0 : 0.0, D.y >= 0.0 ? 1.0 : 0.0);
    D = sqrt( max(D,0.0) );
    f4Intersections =   f2RealRootMask.xxyy * float4(-B - D.x, -B + D.x, -B - D.y, -B + D.y) / (2.0*A) +
                      (float4(1.0, 1.0, 1.0, 1.0) - f2RealRootMask.xxyy) * float4(-1.0, -1.0, -1.0, -1.0);
}

float3 LookUpPrecomputedScattering(in float3 f3StartPoint,
                                   in float3 f3ViewDir,
                                   in float3 f3EarthCentre,
                                   in float  fEarthRadius,
                                   in float3 f3DirOnLight,
                                   in float  fAtmBottomAltitude,
                                   in float  fAtmTopAltitude,
                                   in sampler3D tex3DScatteringLUT,
                                   in SamplerState      tex3DScatteringLUT_sampler,
                                   inout float4 f4UVWQ)
{
    float3 f3EarthCentreToPointDir = f3StartPoint - f3EarthCentre;
    float fDistToEarthCentre = length(f3EarthCentreToPointDir);
    f3EarthCentreToPointDir /= fDistToEarthCentre;
    float fAltitude          = fDistToEarthCentre - fEarthRadius; // Height above sea level
    float fCosViewZenithAngle = dot( f3EarthCentreToPointDir, f3ViewDir    );
    float fCosSunZenithAngle  = dot( f3EarthCentreToPointDir, f3DirOnLight );
    float fCosSunViewAngle    = dot( f3ViewDir,               f3DirOnLight );

    // Provide previous look-up coordinates
    f4UVWQ = WorldParams2InsctrLUTCoords(fAltitude,
                                         fCosViewZenithAngle,
                                         fCosSunZenithAngle,
                                         fCosSunViewAngle,
                                         fEarthRadius,
                                         fAtmBottomAltitude,
                                         fAtmTopAltitude,
                                         f4UVWQ);

    float3 f3UVW0;
    f3UVW0.xy = f4UVWQ.xy;
    float fQ0Slice = floor(f4UVWQ.w * PRECOMPUTED_SCTR_LUT_DIM.w - 0.5);
    fQ0Slice = clamp(fQ0Slice, 0.0, PRECOMPUTED_SCTR_LUT_DIM.w-1.0);
    float fQWeight = (f4UVWQ.w * PRECOMPUTED_SCTR_LUT_DIM.w - 0.5) - fQ0Slice;
    fQWeight = max(fQWeight, 0.0);
    float2 f2SliceMinMaxZ = float2(fQ0Slice, fQ0Slice+1.0)/PRECOMPUTED_SCTR_LUT_DIM.w + float2(0.5,-0.5) / (PRECOMPUTED_SCTR_LUT_DIM.z*PRECOMPUTED_SCTR_LUT_DIM.w);
    f3UVW0.z =  (fQ0Slice + f4UVWQ.z) / PRECOMPUTED_SCTR_LUT_DIM.w;
    f3UVW0.z = clamp(f3UVW0.z, f2SliceMinMaxZ.x, f2SliceMinMaxZ.y);

    float fQ1Slice = min(fQ0Slice+1.0, PRECOMPUTED_SCTR_LUT_DIM.w-1.0);
    float fNextSliceOffset = (fQ1Slice - fQ0Slice) / PRECOMPUTED_SCTR_LUT_DIM.w;
    float3 f3UVW1 = f3UVW0 + float3(0.0, 0.0, fNextSliceOffset);
    float3 f3Insctr0 = SampleLevel_3( tex3DScatteringLUT,tex3DScatteringLUT_sampler/*LinearClamp*/, f3UVW0, 0.0)_SWIZZLE3;
    float3 f3Insctr1 = SampleLevel_3( tex3DScatteringLUT,tex3DScatteringLUT_sampler/*LinearClamp*/, f3UVW1, 0.0)_SWIZZLE3;
    float3 f3Inscattering = lerp(f3Insctr0, f3Insctr1, fQWeight);

    return f3Inscattering;
}


uniform sampler3D g_tex3DPointwiseSctrRadiance;
SamplerState g_tex3DPointwiseSctrRadiance_sampler;

layout(rgba32f, binding=0)uniform IMAGE_WRITEONLY image3D g_rwtex3DInsctrOrder;
layout ( local_size_x = THREAD_GROUP_SIZE, local_size_y = THREAD_GROUP_SIZE, local_size_z = 1 ) in;
void main()
{
    uint3 ThreadId;
    _GET_GL_GLOBAL_INVOCATION_ID(uint3,ThreadId);

    // Get attributes for the current point
    float4 f4LUTCoords = LUTCoordsFromThreadID(ThreadId);
    float fAltitude, fCosViewZenithAngle, fCosSunZenithAngle, fCosSunViewAngle;
    InsctrLUTCoords2WorldParams(f4LUTCoords,
                                g_MediaParams.fEarthRadius,
                                g_MediaParams.fAtmBottomAltitude,
                                g_MediaParams.fAtmTopAltitude,
                                fAltitude,
                                fCosViewZenithAngle,
                                fCosSunZenithAngle,
                                fCosSunViewAngle);
    float3 f3EarthCentre = float3(0.0, -g_MediaParams.fEarthRadius, 0.0);
    float3 f3RayStart    = float3(0.0, fAltitude, 0.0);
    float3 f3ViewDir     = ComputeViewDir(fCosViewZenithAngle);
    float3 f3DirOnLight  = ComputeLightDir(f3ViewDir, fCosSunZenithAngle, fCosSunViewAngle);

    // Intersect the ray with the atmosphere boundaries
    float4 f4Isecs;
    GetRaySphereIntersection2( f3RayStart, f3ViewDir, f3EarthCentre,
                               float2(g_MediaParams.fAtmBottomRadius, g_MediaParams.fAtmTopRadius),
                               f4Isecs);
    float2 f2RayEarthIsecs  = f4Isecs.xy;
    float2 f2RayAtmTopIsecs = f4Isecs.zw;

    if(f2RayAtmTopIsecs.y <= 0.0)
    {
        // This is just a sanity check and should never happen
        // as the start point is always under the top of the
        // atmosphere (look at InsctrLUTCoords2WorldParams())
        imageStore( g_rwtex3DInsctrOrder, _ToIvec(ThreadId), _ExpandVector( float4(0.0, 0.0, 0.0, 0.0)));
        return;
    }

    float fRayLength = f2RayAtmTopIsecs.y;
    if(f2RayEarthIsecs.x > 0.0)
        fRayLength = min(fRayLength, f2RayEarthIsecs.x);

    float3 f3RayEnd = f3RayStart + f3ViewDir * fRayLength;

    const int iNumSamples = 64;

    float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
    float3 f3PrevSctrRadiance = LookUpPrecomputedScattering(
        f3RayStart,
        f3ViewDir,
        f3EarthCentre,
        g_MediaParams.fEarthRadius,
        f3DirOnLight.xyz,
        g_MediaParams.fAtmBottomAltitude,
        g_MediaParams.fAtmTopAltitude,
        g_tex3DPointwiseSctrRadiance,
        g_tex3DPointwiseSctrRadiance_sampler,
        f4UVWQ);
    float2 f2PrevParticleDensity = exp( -float2(fAltitude, fAltitude) * g_MediaParams.f4ParticleScaleHeight.zw );

    float2 f2NetParticleDensityFromCam = float2(0.0, 0.0);
    float3 f3Inscattering = float3(0.0, 0.0, 0.0);

    // We want to place more samples when the starting point is close to the surface,
    // but for high altitudes linear distribution works better.
    float fStartAltitude = length(f3RayStart - f3EarthCentre) - g_MediaParams.fEarthRadius;
    float pwr = lerp(2.0, 1.0, saturate((fStartAltitude - g_MediaParams.fAtmBottomAltitude) * g_MediaParams.fAtmAltitudeRangeInv));

    float fPrevSampleDist = 0.0;
    for (int iSample = 1; iSample <= iNumSamples; ++iSample)
    {
        // We need to place more samples closer to the start point and fewer samples farther away.
        // The ad-hoc power function does the job well.
        float r = pow(float(iSample) / float(iNumSamples), pwr);
        float3 f3Pos = lerp(f3RayStart, f3RayEnd, r);
        float fCurrSampleDist = fRayLength * r;
        float fStepLen = fCurrSampleDist - fPrevSampleDist;
        fPrevSampleDist = fCurrSampleDist;

        float fCurrHeight = length(f3Pos - f3EarthCentre) - g_MediaParams.fEarthRadius;
        float2 f2ParticleDensity = exp( -float2(fCurrHeight, fCurrHeight) * g_MediaParams.f4ParticleScaleHeight.zw );

        f2NetParticleDensityFromCam += (f2PrevParticleDensity + f2ParticleDensity) * (fStepLen / 2.0);
        f2PrevParticleDensity = f2ParticleDensity;

        // Get optical depth
        float3 f3RlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2NetParticleDensityFromCam.x;
        float3 f3MieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2NetParticleDensityFromCam.y;

        // Compute extinction from the camera for the current integration point:
        float3 f3ExtinctionFromCam = exp( -(f3RlghOpticalDepth + f3MieOpticalDepth) );

        // Get attenuated scattered light radiance in the current point
        float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
        float3 f3SctrRadiance = f3ExtinctionFromCam *
            LookUpPrecomputedScattering(
                f3Pos,
                f3ViewDir,
                f3EarthCentre,
                g_MediaParams.fEarthRadius,
                f3DirOnLight.xyz,
                g_MediaParams.fAtmBottomAltitude,
                g_MediaParams.fAtmTopAltitude,
                g_tex3DPointwiseSctrRadiance,
                g_tex3DPointwiseSctrRadiance_sampler,
                f4UVWQ);
        // Update in-scattering integral
        f3Inscattering += (f3SctrRadiance +  f3PrevSctrRadiance) * (fStepLen/2.0);
        f3PrevSctrRadiance = f3SctrRadiance;
    }

    imageStore( g_rwtex3DInsctrOrder, _ToIvec(ThreadId), _ExpandVector( float4(f3Inscattering, 0.0)));
}