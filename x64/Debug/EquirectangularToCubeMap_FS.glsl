#version 430 core
/*
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb; 
    FragColor = vec4(color, 1.0);
}
*/

#define PI 3.1415926

layout(location=0) out vec3 output_outx;
layout(location=1) out vec3 output_outy;
layout(location=2) out vec3 output_outz;

in vec2 TexCoords;
uniform sampler2D equirectangularMap;
uniform float frame_side;

highp vec2 toEquirect(const highp vec3 s) 
{
    highp float xf = atan(s.x, s.z) * (1.0 / PI);   // range [-1.0, 1.0]
    highp float yf = asin(s.y) * (2.0 / PI);        // range [-1.0, 1.0]
    xf = (xf + 1.0) * 0.5;                          // range [0, 1.0]
    yf = (1.0 - yf) * 0.5;                          // range [0, 1.0]
    return vec2(xf, yf);
}

mediump vec3 sampleEquirect(mediump sampler2D equirect, const highp vec3 r) 
{
    mediump vec3 c = texture(equirect, toEquirect(r)).rgb;
    return clamp(c, 0.0, 65536.0); // clamp to get rid of possible +inf
}

void main() 
{
    highp vec2 uv = vec2(TexCoords.x, TexCoords.y); // interpolated at pixel's center
    highp vec2 p = vec2(
        uv.x * 2.0 - 1.0,
        1.0 - uv.y * 2.0);

    float side = frame_side;
    highp float l = inversesqrt(p.x * p.x + p.y * p.y + 1.0);

    // compute the view (and normal, since v = n) direction for each face
    highp vec3 rx = vec3(      side,  p.y,  side * -p.x);
    highp vec3 ry = vec3(       p.x,  side, side * -p.y);
    highp vec3 rz = vec3(side * p.x,  p.y,  side);


    output_outx = sampleEquirect(equirectangularMap, rx * l);
    output_outy = sampleEquirect(equirectangularMap, ry * l);
    output_outz = sampleEquirect(equirectangularMap, rz * l);
}