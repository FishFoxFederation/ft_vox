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
	// debugPrintfEXT("mat %d: %f %f %f %f\n       %f %f %f %f\n        %f %f %f %f\n        %f %f %f %f\n", gl_InvocationID,
	// 	lightSpaceMatrices[gl_InvocationID][0][0], lightSpaceMatrices[gl_InvocationID][0][1], lightSpaceMatrices[gl_InvocationID][0][2], lightSpaceMatrices[gl_InvocationID][0][3],
	// 	lightSpaceMatrices[gl_InvocationID][1][0], lightSpaceMatrices[gl_InvocationID][1][1], lightSpaceMatrices[gl_InvocationID][1][2], lightSpaceMatrices[gl_InvocationID][1][3],
	// 	lightSpaceMatrices[gl_InvocationID][2][0], lightSpaceMatrices[gl_InvocationID][2][1], lightSpaceMatrices[gl_InvocationID][2][2], lightSpaceMatrices[gl_InvocationID][2][3],
	// 	lightSpaceMatrices[gl_InvocationID][3][0], lightSpaceMatrices[gl_InvocationID][3][1], lightSpaceMatrices[gl_InvocationID][3][2], lightSpaceMatrices[gl_InvocationID][3][3]);
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}
