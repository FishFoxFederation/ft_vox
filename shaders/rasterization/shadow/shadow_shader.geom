#version 450 core

#extension GL_EXT_debug_printf : enable

#include "common.glsl"

// layout(triangles, invocations = SHADOW_MAP_MAX_COUNT) in;
layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
	ShadowMapLight shadow_map_light;
};

layout(push_constant) uniform PushConstants
{
	GlobalPushConstant pc;
};

layout(location = 0) in vec3 vert_tex_coords[];

layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = shadow_map_light.view_proj[pc.layer] * gl_in[i].gl_Position;

		frag_tex_coords = vert_tex_coords[i];

		EmitVertex();
	}
	EndPrimitive();
}
