#version 450 core

#extension GL_EXT_debug_printf : enable

#include "common.glsl"

layout(triangles, invocations = SHADOW_MAP_MAX_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
	ShadowMapLight shadow_map_light;
};

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = shadow_map_light.view_proj[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}
