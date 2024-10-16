#version 450

#include "common.glsl"

layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_PARAMS_BINDING) uniform bindlessParams
{
	BindlessDescriptorParams bindless_params;
};
layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_UNIFORM_BUFFER_BINDING) uniform CameraMatrices
{
	ViewProjMatrices cm;
} camera_matrices[BINDLESS_DESCRIPTOR_MAX_COUNT];
layout(push_constant) uniform PushConstants
{
	ModelMatrice pc;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 rayDir;

void main()
{
	const ViewProjMatrices cam = camera_matrices[bindless_params.camera_ubo_index].cm;
	vec4 pos = cam.proj * cam.view * pc.model * vec4(position, 1.0);
	gl_Position = pos.xyww;

	rayDir = normalize(position);
}
