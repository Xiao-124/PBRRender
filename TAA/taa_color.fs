#version 450 core
layout(location=0) out vec3 gPos;
layout(location=1) out vec4 gAlbedo;
layout(location=2) out vec2 gVelo;

in vec4 nowScreenPosition;
in vec4 preScreenPosition;
void main()
{
    // position/normal/alebdo等
	gPos = vec3(0.0f,0.0f,0.0f);
    gAlbedo = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	
    // velocity
    vec2 newPos = ((nowScreenPosition.xy / nowScreenPosition.w) * 0.5 + 0.5);
    vec2 prePos = ((preScreenPosition.xy / preScreenPosition.w) * 0.5 + 0.5);
    gVelo = newPos - prePos;
}