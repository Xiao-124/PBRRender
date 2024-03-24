
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


void _TypeConvertStore( out float Dst, in int   Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in uint  Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in float Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in bool  Src ){ Dst = Src ? 1.0 : 0.0; }

void _TypeConvertStore( out uint  Dst, in int   Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in uint  Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in float Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in bool  Src ){ Dst = Src ? 1u : 0u; }

void _TypeConvertStore( out int   Dst, in int   Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in uint  Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in float Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in bool  Src ){ Dst = Src ? 1 : 0; }

void _TypeConvertStore( out bool  Dst, in int   Src ){ Dst = (Src != 0);   }
void _TypeConvertStore( out bool  Dst, in uint  Src ){ Dst = (Src != 0u);  }
void _TypeConvertStore( out bool  Dst, in float Src ){ Dst = (Src != 0.0); }
void _TypeConvertStore( out bool  Dst, in bool  Src ){ Dst = Src;          }

#define PI 3.141592653
#define _GET_GL_VERTEX_ID(VertexId)  _TypeConvertStore(VertexId, gl_VertexID)
#define MATRIX_ELEMENT(mat, row, col) mat[col][row]
#define fSunAngularRadius (32.0/2.0 / 60.0 * ((2.0 * PI)/180.0)) // Sun angular DIAMETER is 32 arc minutes
#define fTanSunAngularRadius tan(fSunAngularRadius)

out vec2 f2NormalizedXY;

uniform mat4 mProj;
uniform vec4 f4LightScreenPos;

void main()
{
    uint VertexId;
    _GET_GL_VERTEX_ID(VertexId);


    float4 f4Pos;
    float2 fCotanHalfFOV = float2( MATRIX_ELEMENT(mProj, 0, 0), MATRIX_ELEMENT(mProj, 1, 1) );
    fCotanHalfFOV = vec2(1.0f, 0.8f);
    //fCotanHalfFOV = vec2(1280.0f, 760.0f);

    float2 f2SunScreenPos = f4LightScreenPos.xy;
    float2 f2SunScreenSize = fTanSunAngularRadius * fCotanHalfFOV;

    //float4 MinMaxUV = f2SunScreenPos.xyxy + float4(-1.0, -1.0, 1.0, 1.0) * f2SunScreenSize.xyxy;
    float4 MinMaxUV = f2SunScreenPos.xyxy + float4(-1.0, -1.0, 1.0, 1.0) * f2SunScreenSize.xyxy;

    float2 Verts[4];
    Verts[0] = MinMaxUV.xy;
    Verts[1] = MinMaxUV.xw;
    Verts[2] = MinMaxUV.zy;
    Verts[3] = MinMaxUV.zw;

    f2NormalizedXY = Verts[VertexId];
    f4Pos = float4(Verts[VertexId], 1.0, 1.0);
    gl_Position = f4Pos;

}