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
layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING) uniform sampler2DArray sampler_2d_array[BINDLESS_DESCRIPTOR_MAX_COUNT];


layout(location = 0) in vec3 frag_tex_coords;

void main()
{
	vec4 color = texture(sampler_2d_array[bindless_params.block_texture_index], frag_tex_coords);

	if (color.a < 0.01)
		discard;
}
