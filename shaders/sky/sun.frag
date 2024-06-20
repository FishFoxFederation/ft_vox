#version 450

layout(push_constant) uniform PushConstants
{
	mat4 model;
	vec3 sunDir;
}pc;

layout(location = 0) in vec3 rayDir;

layout(location = 0) out vec4 outColor;

void main()
{
	float dotProduct = dot(rayDir, pc.sunDir);
	float sunIntensity = pow(2.0, 50.0 * (dotProduct - 1.0));

	vec3 sunColor = vec3(1.0, 0.9, 0.5);
	vec3 skyColor = vec3(0.2, 0.3, 1.0);

	outColor = vec4(mix(skyColor, sunColor, sunIntensity), 1.0);
}
