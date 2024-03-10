#version 430 core

#extension GL_NV_shader_atomic_float : require
#extension GL_NV_shader_atomic_fp16_vector : require
#extension GL_NV_gpu_shader5 : require

precision highp float;
precision highp samplerCube;



#define LOCAL_GROUP_SIZE 16
layout (local_size_x = LOCAL_GROUP_SIZE, local_size_y = LOCAL_GROUP_SIZE) in;

uniform samplerCube u_CubeMap;	

layout (std430, binding = 0) buffer SH
{
	float u_SHs[27];
};

float sphereQuadrantArea(float x, float y) 
{
    return atan(x * y, sqrt(x * x + y * y + 1));
}

float solidAngle(int dim, int u, int v) 
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

vec3 getDirectionFor(int face, float x, float y) 
{
    // map [0, dim] to [-1,1] with (-1,-1) at bottom left

    float mScale = 2.0f / 256.0f;
    float cx = (x * mScale) - 1;
    float cy = 1 - (y * mScale);

    vec3 dir;
    const float l = sqrt(cx * cx + cy * cy + 1);
    switch (face) 
    {    
        case 0:  dir = vec3( 1, cy,  -cx ); break;
        case 1:  dir = vec3( -1, cy,  cx ); break;
        case 2:  dir = vec3( cx,  1, -cy ); break;
        case 3:  dir = vec3( cx, -1,  cy ); break;
        case 4:  dir = vec3( cx, cy,   1 ); break;
        case 5:  dir = vec3( -cx, cy, -1 ); break;
    }
    return dir * (1.0f / l);
}

void getBasis(const vec3 pos, out float Y[9])
{
    float PI = 3.14159265453;
    vec3 normal = normalize(pos);
    float x = normal.x;
    float y = normal.y;
    float z = normal.z;

    Y[0] = 1.f / 2.f * sqrt(1.f / PI);
    Y[1] = sqrt(3.f / (4.f * PI)) * z;
    Y[2] = sqrt(3.f / (4.f * PI)) * y;
    Y[3] = sqrt(3.f / (4.f * PI)) * x;
    Y[4] = 1.f / 2.f * sqrt(15.f / PI) * x * z;
    Y[5] = 1.f / 2.f * sqrt(15.f / PI) * z * y;
    Y[6] = 1.f / 4.f * sqrt(5.f / PI) * (-x * x - z * z + 2 * y * y);
    Y[7] = 1.f / 2.f * sqrt(15.f / PI) * y * x;
    Y[8] = 1.f / 4.f * sqrt(15.f / PI) * (x * x - z * z);    
}

void main()
{
    float shbais[9];

	ivec3 samplePos = ivec3(gl_GlobalInvocationID.xyz);
    vec3 dir = getDirectionFor(samplePos.z, samplePos.x, samplePos.y);
    dir = normalize(dir);
    vec3 color = textureLod(u_CubeMap, dir, 0).rgb;

    color *= solidAngle(256, samplePos.x, samplePos.y);
    getBasis(dir, shbais);              
    for (int i = 0; i < 9; i++) 
    {
        //u_SHs[i] += color * shbais[i];
        //u_SHs[i*3] += color.x * shbais[i];
        //u_SHs[i*3+1] += color.y * shbais[i];
        //u_SHs[i*3+2] += color.z * shbais[i];

        atomicAdd(u_SHs[i*3],   color.x * shbais[i]);
        atomicAdd(u_SHs[i*3+1], color.y * shbais[i]);
        atomicAdd(u_SHs[i*3+2], color.z * shbais[i]);
    }

}

