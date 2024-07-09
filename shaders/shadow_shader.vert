#version 450

layout(set = 0, binding = 0) uniform LightMatrices
{
	mat4 view;
	mat4 proj;
}lm;

layout(push_constant) uniform PushConstants
{
	mat4 model;
}pc;

layout(location = 0) in vec3 positions;

void main()
{
    gl_Position = lm.proj * lm.view * pc.model * vec4(positions, 1.0);
}
