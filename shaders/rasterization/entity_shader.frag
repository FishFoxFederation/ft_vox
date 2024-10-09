#version 450

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	EntityMatrices pc;
};

layout(location = 0) in vec3 fragNorm;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 fragNormal = abs(fragNorm);
    float intensity = fragNormal.x * 0.3 + fragNormal.y * 0.5 + fragNormal.z * 0.7;
	outColor = vec4(pc.color.rgb * intensity, pc.color.a);
}
