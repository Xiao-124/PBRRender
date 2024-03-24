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


uniform sampler3D g_tex3DSingleSctrLUT;
uniform sampler3D g_tex3DHighOrderSctrLUT;

layout(rgba16f, binding=0)uniform IMAGE_WRITEONLY image3D g_rwtex3DMultipleSctr;

layout ( local_size_x = THREAD_GROUP_SIZE, local_size_y = THREAD_GROUP_SIZE, local_size_z = 1 ) in;

void main()
{
    uint3 ThreadId;
    _GET_GL_GLOBAL_INVOCATION_ID(uint3,ThreadId);

    // Combine single & higher order scattering into single look-up table
    float3 f3MultipleSctr = LoadTex3D_1( g_tex3DSingleSctrLUT, int4(ThreadId, 0) )_SWIZZLE3 +
                            LoadTex3D_1( g_tex3DHighOrderSctrLUT, int4(ThreadId, 0) )_SWIZZLE3;
    imageStore( g_rwtex3DMultipleSctr, _ToIvec(ThreadId), _ExpandVector( float4(f3MultipleSctr, 0.0)));
}