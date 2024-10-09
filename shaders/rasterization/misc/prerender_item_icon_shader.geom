#version 450 core

#include "common.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(push_constant) uniform PushConstants
{
	PreRenderItemIconPushConstant pc;
};

layout(location = 0) in vec3 vert_normal[];
layout(location = 1) in vec3 vert_tex_coords[];

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_tex_coords;

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = gl_in[i].gl_Position;
		gl_Layer = pc.layer;

		frag_normal = vert_normal[i];
		frag_tex_coords = vert_tex_coords[i];

		EmitVertex();
	}
	EndPrimitive();
}
