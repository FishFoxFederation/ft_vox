// Commun file for all shaders

#include "../src/app/vulkan/ShaderCommon.hpp"

#extension GL_ARB_gpu_shader_int64 : enable

vec3 getNormalVector(int normal_id)
{
	// the order is define in Block.hpp
	const vec3 normal[6] = {
		vec3(0.0, 1.0, 0.0), // top
		vec3(0.0, -1.0, 0.0), // bottom
		vec3(1.0, 0.0, 0.0), // right
		vec3(-1.0, 0.0, 0.0), // left
		vec3(0.0, 0.0, 1.0), // front
		vec3(0.0, 0.0, -1.0) // back
	};
	return normal[normal_id];
}

void extractBlockVertexData(
	in u64vec2 vertexData,
	out vec3 position,
	out vec3 normal,
	out vec2 uv,
	out uint texLayer,
	out uint ao,
	out uint light
)
{
	position.x = float(vertexData[0] & 0x1F); // 5 bits
	position.y = float((vertexData[0] >> 5) & 0x3FF); // 10 bits
	position.z = float((vertexData[0] >> 15) & 0x1F); // 5 bits

	normal = getNormalVector(int((vertexData[0] >> 20) & 0x7)); // 3 bits

	uv.x = float((vertexData[0] >> 23) & 0x1F); // 5 bits
	uv.y = float((vertexData[0] >> 28) & 0x1F); // 5 bits

	light = uint((vertexData[0] >> 33) & 0xFF); // 8 bits

	ao = uint((vertexData[0] >> 41) & 0x3); // 2 bits

	texLayer = uint(vertexData[1] & 0xFFFFFFFF); // 32 bits
}
