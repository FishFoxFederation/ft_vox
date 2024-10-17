#version 450 core

#extension GL_EXT_scalar_block_layout: enable

#include "common.glsl"

layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_PARAMS_BINDING) uniform bindlessParams
{
	BindlessDescriptorParams bindless_params;
};

layout(set = BINDLESS_DESCRIPTOR_SET, binding = BINDLESS_UNIFORM_BUFFER_BINDING) uniform ShadowMapLightDescriptor
{
	ShadowMapLight data;
} shadow_map_light_descriptor[BINDLESS_DESCRIPTOR_MAX_COUNT];

layout(triangles, invocations = SHADOW_MAP_MAX_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;


layout(location = 0) in vec3 vert_tex_coords[];

layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	ShadowMapLight shadow_map_light = shadow_map_light_descriptor[bindless_params.light_matrices_index].data;

	for (int i = 0; i < 3; ++i)
	{
		gl_Position = shadow_map_light.view_proj[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;

		frag_tex_coords = vert_tex_coords[i];

		EmitVertex();
	}
	EndPrimitive();
}
