#version 450

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;

layout(push_constant) uniform PushConstants
{
	mat4 model;
}pc;

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;

layout(location = 0) out vec3 fragColors;

void main()
{
    gl_Position = cm.projection * cm.view * pc.model * vec4(positions, 1.0);
    fragColors = normals;
}
