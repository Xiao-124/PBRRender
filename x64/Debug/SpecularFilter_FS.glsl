#version 430 core

#extension GL_ARB_shading_language_packing : enable

#extension GL_GOOGLE_cpp_style_line_directive : enable

precision highp float;
precision highp int;

#define PI 3.1415926
layout(location=0) out vec3 output_outx;
layout(location=1) out vec3 output_outy;
layout(location=2) out vec3 output_outz;

in vec2 TexCoords;

uniform samplerCube materialParams_environment;
uniform sampler2D materialParams_kernel;
uniform vec2 frame_compress;
uniform float frame_lodOffset;
uniform int frame_sampleCount;
uniform int frame_attachmentLevel;
uniform float frame_side;

mat3 tangentSpace(const vec3 N) 
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    mat3 R;
    R[0] = normalize(cross(up, N));
    R[1] = cross(N, R[0]);
    R[2] = N;
    return R;
}

float random(const highp vec2 w) 
{
    const vec3 m = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(m.z * fract(dot(w, m.xy)));
}

// See: http://graphicrants.blogspot.com/2013/12/tone-mapping.html, By Brian Karis
vec3 compress(const vec3 color, const float linear, const float compressed) 
{
    const mediump vec3 rec709 = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(color, rec709);  // REC 709
    float s = 1.0;
    if (luma > linear) 
    {
        s = ((linear * linear - compressed * luma) / ((2.0 * linear - compressed - luma) * luma));
    }
    return color * s;
}

vec3 sampleEnvironment(mediump samplerCube environment, highp vec3 r, mediump float lod) 
{
    mediump float linear = frame_compress.x;
    mediump float compressed = frame_compress.y;
    mediump vec3 c = textureLod(environment, r, lod).rgb;
    //return c;
    return compress(c, linear, compressed);
    // return c * (compressed / (compressed + luma));       // cheaper
    // return clamp(c, 0.0, compressed);                    // cheapest
}




void main() 
{
    vec2 uv = vec2(TexCoords.x, TexCoords.y); // interpolated at pixel's center
    vec2 p = uv * 2.0 - 1.0;

    float side = frame_side;
    
    // compute the view (and normal, since v = n) direction for each face
    vec3 rx = normalize(vec3(      side,  -p.y, side * -p.x));
    vec3 ry = normalize(vec3(       p.x,  side, side *  p.y));
    vec3 rz = normalize(vec3(side * p.x,  -p.y, side));
    
    // random rotation around r
    mediump float a = 2.0 * PI * random(gl_FragCoord.xy);
    mediump float c = cos(a);
    mediump float s = sin(a);
    mat3 R;
    R[0] = vec3( c, s, 0);
    R[1] = vec3(-s, c, 0);
    R[2] = vec3( 0, 0, 1);
    
    // compute the rotation by which to transform our sample locations for each face
    mat3 Tx = tangentSpace(rx) * R;
    mat3 Ty = tangentSpace(ry) * R;
    mat3 Tz = tangentSpace(rz) * R;
    
    // accumulated environment light for each face
    vec3 Lx = vec3(0);
    vec3 Ly = vec3(0);
    vec3 Lz = vec3(0);
    
    float kernelWeight = 0.0;  
    //for (uint i = 0u; i < frame_sampleCount; i++) 
    for (uint i = 0u; i < frame_sampleCount; i++) 
    {
        // { L, lod }, with L.z == NoL
        mediump vec4 entry = texelFetch(materialParams_kernel, ivec2(frame_attachmentLevel, i), 0);
        if (entry.z > 0.0) 
        {
            float l = entry.w + frame_lodOffset;// we don't need to clamp, the h/w does it for us
            Lx += sampleEnvironment(materialParams_environment, Tx * entry.xyz, l) * entry.z;
            Ly += sampleEnvironment(materialParams_environment, Ty * entry.xyz, l) * entry.z;
            Lz += sampleEnvironment(materialParams_environment, Tz * entry.xyz, l) * entry.z;
            kernelWeight += entry.z;
        }
    }
    
    float invKernelWeight = 1.0 / kernelWeight;
    Lx *= invKernelWeight;
    Ly *= invKernelWeight;
    Lz *= invKernelWeight;
    
    output_outx = Lx;
    output_outy = Ly;
    output_outz = Lz;

    //output_outx = vec3(1.0f,0.0f,0.0f);
    //output_outy = vec3(1.0f,0.0f,0.0f);
    //output_outz = vec3(1.0f,0.0f,0.0f);

    //vec3 N = normalize(WorldPos);

    // make the simplifying assumption that V equals R equals the normal 
    //vec3 R = N;
    //vec3 V = R;

    //mediump float a = 2.0 * PI * random(gl_FragCoord.xy);
    //mediump float c = cos(a);
    //mediump float s = sin(a);
    //mat3 R;
    //R[0] = vec3( c, s, 0);
    //R[1] = vec3(-s, c, 0);
    //R[2] = vec3( 0, 0, 1);
    //
    //// compute the rotation by which to transform our sample locations for each face
    //mat3 Tx = tangentSpace(N) * R;
    //vec3 Lx = vec3(0);
    //float kernelWeight = 0.0;
    //for (uint i = 0u; i < frame_sampleCount; i++) 
    //{
    //    // { L, lod }, with L.z == NoL
    //    mediump vec4 entry = texelFetch(materialParams_kernel, ivec2(frame_attachmentLevel, i), 0);
    //    if (entry.z > 0.0) 
    //    {
    //        float l = entry.w + frame_lodOffset;// we don't need to clamp, the h/w does it for us
    //        Lx += sampleEnvironment(materialParams_environment, Tx * entry.xyz, l) * entry.z;
    //        kernelWeight += entry.z;
    //    }
    //}
    //float invKernelWeight = 1.0 / kernelWeight;
    //Lx *= invKernelWeight;
    //FragColor = vec4(Lx, 1.0f);
    //FragColor = vec4(1.0f,0.0f,0.0f,1.0f);
}

