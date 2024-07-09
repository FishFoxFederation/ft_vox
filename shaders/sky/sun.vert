#version 450

// Vertex shader for the skybox

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;

layout(push_constant) uniform PushConstants
{
	mat4 model;
}pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 rayDir;

void main()
{
	vec4 pos = cm.projection * cm.view * pc.model * vec4(position, 1.0);
	gl_Position = pos.xyww;

	rayDir = normalize(position);
}
