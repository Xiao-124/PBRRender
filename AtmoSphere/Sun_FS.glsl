
#version 450 core

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

in float2 f2NormalizedXY;
out vec4 f4Color;

#define PI 3.141592653
#define MATRIX_ELEMENT(mat, row, col) mat[col][row]
#define fSunAngularRadius (32.0/2.0 / 60.0 * ((2.0 * PI)/180.0)) // Sun angular DIAMETER is 32 arc minutes
#define fTanSunAngularRadius tan(fSunAngularRadius)


float saturate( float x ){ return clamp( x, 0.0,                      1.0 ); }
vec2  saturate( vec2  x ){ return clamp( x, vec2(0.0, 0.0),           vec2(1.0, 1.0) ); }
vec3  saturate( vec3  x ){ return clamp( x, vec3(0.0, 0.0, 0.0),      vec3(1.0, 1.0, 1.0) ); }
vec4  saturate( vec4  x ){ return clamp( x, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0) ); }


uniform mat4 mProj;
uniform vec4 f4LightScreenPos;

void main()
{

    float2 fCotanHalfFOV = float2( MATRIX_ELEMENT(mProj, 0, 0), MATRIX_ELEMENT(mProj, 1, 1) );


    //fCotanHalfFOV = vec2(1.0f, 0.8f);
    float2 f2SunScreenSize = fTanSunAngularRadius * fCotanHalfFOV;
    float2 f2dXY = (f2NormalizedXY - f4LightScreenPos.xy) / f2SunScreenSize;


    f4Color.rgb = sqrt(saturate(1.0 - dot(f2dXY, f2dXY))) * float3(1.0, 1.0, 1.0);
    f4Color.a = 1.0f;
    //f4Color = vec4(1.0f,0.0f,0.0f,1.0f);

}