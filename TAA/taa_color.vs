#version 450 core
layout (location = 0) in vec3 aPos;

uniform float screenWidth;
uniform float screenHeight;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int offsetIdx;



uniform mat4 preProjection;
uniform mat4 preView;
uniform mat4 preModel;


out vec4 preScreenPosition;
out vec4 nowScreenPosition;
out vec3 position;

const vec2 Halton_2_3[8] =
{
    vec2(0.0f, -1.0f / 3.0f),
    vec2(-1.0f / 2.0f, 1.0f / 3.0f),
    vec2(1.0f / 2.0f, -7.0f / 9.0f),
    vec2(-3.0f / 4.0f, -1.0f / 9.0f),
    vec2(1.0f / 4.0f, 5.0f / 9.0f),
    vec2(-1.0f / 4.0f, -5.0f / 9.0f),
    vec2(3.0f / 4.0f, 1.0f / 9.0f),
    vec2(-7.0f / 8.0f, 7.0f / 9.0f)
};

void main()
{
	

    float deltaWidth = 1.0/screenWidth, deltaHeight = 1.0/screenHeight;
    vec2 jitter = vec2(
        Halton_2_3[offsetIdx].x * deltaWidth,
        Halton_2_3[offsetIdx].y * deltaHeight
    );
    mat4 jitterMat = projection;
    jitterMat[2][0] += jitter.x;
    jitterMat[2][1] += jitter.y;

    vec3 nowPositon = aPos;


    gl_Position = jitterMat * view * model * vec4(nowPositon, 1.0);
	
	preScreenPosition = preProjection * preView * preModel * vec4(nowPositon, 1.0);
    // 注意这里就不要添加jitter了
    nowScreenPosition = projection * view * model * vec4(nowPositon, 1.0);
    
}