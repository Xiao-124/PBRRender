#version 430 core
layout(location = 0) in vec3 _Position;
layout(location = 1) in vec3 _Normal;
layout(location = 2) in vec2 _TexCoord;
layout(location = 3) in vec3 _Tangent;

layout(std140, binding = 0) uniform u_Matrices4ProjectionWorld
{
	mat4 u_ProjectionMatrix;
	mat4 u_ViewMatrix;
};


void toTangentFrame(const highp vec4 q, out highp vec3 n) 
{
    n = vec3( 0.0,  0.0,  1.0) +
        vec3( 2.0, -2.0, -2.0) * q.x * q.zwx +
        vec3( 2.0,  2.0, -2.0) * q.y * q.wzy;
}

void toTangentFrame(const highp vec4 q, out highp vec3 n, out highp vec3 t) 
{
    toTangentFrame(q, n);
    t = vec3( 1.0,  0.0,  0.0) +
        vec3(-2.0,  2.0, -2.0) * q.y * q.yxw +
        vec3(-2.0,  2.0,  2.0) * q.z * q.zwx;
}

uniform mat4 u_ModelMatrix;
out vec2 v2f_TexCoords;
out vec3 v2f_Normal;
out vec3 v2f_FragPosInViewSpace;
out vec4 vertex_worldTangent;

void main()
{
	vec4 FragPosInViewSpace =  u_ModelMatrix *vec4(_Position, 1.0f);
	gl_Position = u_ProjectionMatrix *u_ViewMatrix *FragPosInViewSpace;

	v2f_TexCoords = _TexCoord;
	v2f_Normal = normalize(mat3(transpose(inverse(u_ModelMatrix))) * _Normal); //这个可以在外面算好了传进来

	vec3 worldNormal;
	toTangentFrame(vec4(_Tangent, 1.0f), worldNormal, vertex_worldTangent.xyz);
	vertex_worldTangent.w = 1.0f;
	v2f_FragPosInViewSpace = vec3(FragPosInViewSpace);

}