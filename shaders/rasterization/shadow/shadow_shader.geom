#version 450 core

#include "common.glsl"

layout(triangles, invocations = SHADOW_MAP_MAX_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, set = 1, binding = 0) uniform LightSpaceMatrices
{
	ShadowMapLight shadow_map_light;
};

layout(location = 0) in vec3 vert_tex_coords[];

layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = shadow_map_light.view_proj[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;

		frag_tex_coords = vert_tex_coords[i];

		EmitVertex();
	}
	EndPrimitive();
}
