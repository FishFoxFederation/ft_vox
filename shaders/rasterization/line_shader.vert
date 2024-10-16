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
	LinePipelinePushConstant pc;
};

layout(location = 0) in vec3 positions;

layout(location = 0) out vec3 fragColors;

void main()
{
    const ViewProjMatrices cam = camera_matrices[bindless_params.camera_ubo_index].cm;
	gl_Position = cam.proj * cam.view * pc.model * vec4(positions, 1.0);
}
