#version 430 core

#define saturate(x)        clamp(x, 0.0, 1.0)
#define PI  3.141592653
in vec2 TexCoords;
out vec3 FragColor;


uniform highp sampler2D materialParams_depth;


uniform vec4 materialParams_resolution;
uniform vec2 materialParams_positionParams;
uniform float materialParams_invRadiusSquared;
uniform float materialParams_minHorizonAngleSineSquared;
uniform float materialParams_peak2;
uniform float materialParams_projectionScale;
uniform float materialParams_projectionScaleRadius;
uniform float materialParams_bias;
uniform float materialParams_power;
uniform float materialParams_intensity;
uniform float materialParams_spiralTurns;
uniform vec2 materialParams_sampleCount;
uniform vec2 materialParams_angleIncCosSin;
uniform float materialParams_invFarPlane;
uniform int materialParams_maxLevel;
//uniform vec2 materialParams_reserved;


uniform float near_plane;
uniform float far_plane;



const float kLog2LodRate = 3.0;

vec2 pack(highp float normalizedDepth) 
{
    // we need 16-bits of precision
    highp float z = clamp(normalizedDepth, 0.0, 1.0);
    highp float t = floor(256.0 * z);
    mediump float hi = t * (1.0 / 256.0);   // we only need 8-bits of precision
    mediump float lo = (256.0 * z) - t;     // we only need 8-bits of precision
    return vec2(hi, lo);
}

float sq(float x) 
{
    return x * x;
}

highp vec2 getFragCoord(const highp vec2 resolution) 
{
    return gl_FragCoord.xy;
}

highp float sampleDepth(const highp sampler2D depthTexture, const highp vec2 uv, float lod) 
{
    return textureLod(depthTexture, uv, lod).r;
}

highp float linearizeDepth(highp float depth) 
{
    // Our far plane is at infinity, which causes a division by zero below, which in turn
    // causes some issues on some GPU. We workaround it by replacing "infinity" by the closest
    // value representable in  a 24 bit depth buffer.
    //const highp float preventDiv0 = 1.0 / 16777216.0;
    //mat4 p = getViewFromClipMatrix();
    //// this works with perspective and ortho projections, for a perspective projection
    //// this resolves to -near/depth, for an ortho projection this resolves to depth*(far - near) - far
    //return (depth * p[2].z + p[3].z) / max(depth * p[2].w + p[3].w, preventDiv0);

    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));

}


highp float sampleDepthLinear(const highp sampler2D depthTexture,
        const highp vec2 uv, float lod) 
{
    return linearizeDepth(sampleDepth(depthTexture, uv, lod));
}

highp vec3 computeViewSpacePositionFromDepth(highp vec2 uv, highp float linearDepth,
        highp vec2 positionParams) 
{
    return vec3((0.5 - uv) * positionParams * linearDepth, linearDepth);
}

highp vec3 faceNormal(highp vec3 dpdx, highp vec3 dpdy) 
{
    return normalize(cross(dpdx, dpdy));
}

highp vec3 computeViewSpaceNormalHighQ(
        const highp sampler2D depthTexture, const highp vec2 uv,
        const highp float depth, const highp vec3 position,
        highp vec2 texel, highp vec2 positionParams) 
{

    precision highp float;

    vec3 pos_c = position;
    highp vec2 dx = vec2(texel.x, 0.0);
    highp vec2 dy = vec2(0.0, texel.y);

    vec4 H;
    H.x = sampleDepth(depthTexture, uv - dx, 0.0);
    H.y = sampleDepth(depthTexture, uv + dx, 0.0);
    H.z = sampleDepth(depthTexture, uv - dx * 2.0, 0.0);
    H.w = sampleDepth(depthTexture, uv + dx * 2.0, 0.0);
    vec2 he = abs((2.0 * H.xy - H.zw) - depth);
    vec3 pos_l = computeViewSpacePositionFromDepth(uv - dx,
            linearizeDepth(H.x), positionParams);
    vec3 pos_r = computeViewSpacePositionFromDepth(uv + dx,
            linearizeDepth(H.y), positionParams);
    vec3 dpdx = (he.x < he.y) ? (pos_c - pos_l) : (pos_r - pos_c);

    vec4 V;
    V.x = sampleDepth(depthTexture, uv - dy, 0.0);
    V.y = sampleDepth(depthTexture, uv + dy, 0.0);
    V.z = sampleDepth(depthTexture, uv - dy * 2.0, 0.0);
    V.w = sampleDepth(depthTexture, uv + dy * 2.0, 0.0);
    vec2 ve = abs((2.0 * V.xy - V.zw) - depth);
    vec3 pos_d = computeViewSpacePositionFromDepth(uv - dy,
            linearizeDepth(V.x), positionParams);
    vec3 pos_u = computeViewSpacePositionFromDepth(uv + dy,
            linearizeDepth(V.y), positionParams);
    vec3 dpdy = (ve.x < ve.y) ? (pos_c - pos_d) : (pos_u - pos_c);

    return faceNormal(dpdx, dpdy);
}


highp vec3 computeViewSpaceNormal(
        const highp sampler2D depthTexture, const highp vec2 uv,
        const highp float depth, const highp vec3 position,
        highp vec2 texel, highp vec2 positionParams) 
{
    vec3 normal = computeViewSpaceNormalHighQ(depthTexture, uv, depth, position,
            texel, positionParams);
    return normal;
}


float interleavedGradientNoise(highp vec2 w) 
{
    const vec3 m = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(m.z * fract(dot(w, m.xy)));
}

highp vec2 startPosition(const float noise) 
{
    float angle = ((2.0 * PI) * 2.4) * noise;
    return vec2(cos(angle), sin(angle));
}


highp mat2 tapAngleStep() 
{
    highp vec2 t = materialParams_angleIncCosSin;
    return mat2(t.x, t.y, -t.y, t.x);
}

vec3 tapLocationFast(float i, vec2 p, const float noise) 
{
    float radius = (i + noise + 0.5) * materialParams_sampleCount.y;
    return vec3(p, radius * radius);
}

void computeAmbientOcclusionSAO(inout float occlusion, inout vec3 bentNormal,
        float i, float ssDiskRadius,
        const highp vec2 uv,  const highp vec3 origin, const vec3 normal,
        const vec2 tapPosition, const float noise) 
{

    vec3 tap = tapLocationFast(i, tapPosition, noise);

    float ssRadius = max(1.0, tap.z * ssDiskRadius); // at least 1 pixel screen-space radius

    vec2 uvSamplePos = uv + vec2(ssRadius * tap.xy) * materialParams_resolution.zw;

    float level = clamp(floor(log2(ssRadius)) - kLog2LodRate, 0.0, float(materialParams_maxLevel));
    highp float occlusionDepth = sampleDepthLinear(materialParams_depth, uvSamplePos, level);
    highp vec3 p = computeViewSpacePositionFromDepth(uvSamplePos, occlusionDepth, materialParams_positionParams);

    // now we have the sample, compute AO
    highp vec3 v = p - origin;  // sample vector
    float vv = dot(v, v);       // squared distance
    float vn = dot(v, normal);  // distance * cos(v, normal)

    // discard samples that are outside of the radius, preventing distant geometry to
    // cast shadows -- there are many functions that work and choosing one is an artistic
    // decision.
    float w = sq(max(0.0, 1.0 - vv * materialParams_invRadiusSquared));

    // discard samples that are too close to the horizon to reduce shadows cast by geometry
    // not sufficently tessellated. The goal is to discard samples that form an angle 'beta'
    // smaller than 'epsilon' with the horizon. We already have dot(v,n) which is equal to the
    // sin(beta) * |v|. So the test simplifies to vn^2 < vv * sin(epsilon)^2.
    w *= step(vv * materialParams_minHorizonAngleSineSquared, vn * vn);

    float sampleOcclusion = max(0.0, vn + (origin.z * materialParams_bias)) / (vv + materialParams_peak2);
    occlusion += w * sampleOcclusion;


}

void scalableAmbientObscurance(out float obscurance, out vec3 bentNormal,
        highp vec2 uv, highp vec3 origin, vec3 normal) 
{

    float noise = interleavedGradientNoise(getFragCoord(materialParams_resolution.xy));
    highp vec2 tapPosition = startPosition(noise);
    highp mat2 angleStep = tapAngleStep();

    // Choose the screen-space sample radius
    // proportional to the projected area of the sphere
    float ssDiskRadius = -(materialParams_projectionScaleRadius / origin.z);

    obscurance = 0.0;
    bentNormal = normal;
    for (float i = 0.0; i < materialParams_sampleCount.x; i += 1.0) 
    {
        computeAmbientOcclusionSAO(obscurance, bentNormal,
                i, ssDiskRadius, uv, origin, normal, tapPosition, noise);
        tapPosition = angleStep * tapPosition;
    }
    obscurance = sqrt(obscurance * materialParams_intensity);

}




void main()
{
    vec2 uv = TexCoords;
    highp float depth = sampleDepth(materialParams_depth, uv, 0.0);
    highp float z = linearizeDepth(depth);
    highp vec3 origin = computeViewSpacePositionFromDepth(uv, z, materialParams_positionParams);

    vec3 normal = computeViewSpaceNormal(materialParams_depth, uv, depth, origin,
            materialParams_resolution.zw,
            materialParams_positionParams);

    float occlusion = 0.0;
    vec3 bentNormal; // will be discarded
    if (materialParams_intensity > 0.0) 
    {
        scalableAmbientObscurance(occlusion, bentNormal, uv, origin, normal);
    }

    //if (materialParams_ssctIntensity > 0.0) 
    //{
    //    occlusion = max(occlusion, dominantLightShadowing(uv, origin, normal));
    //}
    // occlusion to visibility
    float aoVisibility = pow(saturate(1.0 - occlusion), materialParams_power);
    FragColor.rgb = vec3(aoVisibility, pack(origin.z * materialParams_invFarPlane));
    FragColor.rgb = vec3(aoVisibility, aoVisibility, aoVisibility);

}