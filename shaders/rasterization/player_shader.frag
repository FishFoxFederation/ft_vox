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
layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING) uniform sampler2D sampler_2d[BINDLESS_DESCRIPTOR_MAX_COUNT];


layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(sampler_2d[bindless_params.player_texture_index], inUV);
}
