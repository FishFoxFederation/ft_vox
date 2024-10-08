#version 450 core

#extension GL_EXT_debug_printf : enable

#define SHADOW_MAP_COUNT 5

layout(triangles, invocations = SHADOW_MAP_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
	mat4 lightSpaceMatrices[SHADOW_MAP_COUNT];
};

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}
