// Commun file for all shaders

#include "../src/app/vulkan/ShaderCommon.hpp"

#extension GL_EXT_buffer_reference2 : require
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
// #extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct ChunkBlockVertex
{
	u64vec2 data;
};
struct ChunkWaterVertex
{
	u64vec2 data;
};

layout(buffer_reference, std430) readonly buffer ChunkBlockVertexBuffer
{
	ChunkBlockVertex vertices[];
};
layout(buffer_reference, std430) readonly buffer ChunkWaterVertexBuffer
{
	ChunkWaterVertex vertices[];
};

struct InstanceData
{
	mat4 matrice;
	ChunkBlockVertexBuffer block_vextex_buffer;
	ChunkWaterVertexBuffer water_vertex_buffer;
};

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
	in ChunkBlockVertex vertex,
	out vec3 position,
	out vec3 normal,
	out vec2 uv,
	out uint texLayer,
	out uint ao,
	out uint light
)
{
	position.x = float(vertex.data[0] & 0x1F); // 5 bits
	position.y = float((vertex.data[0] >> 5) & 0x3FF); // 10 bits
	position.z = float((vertex.data[0] >> 15) & 0x1F); // 5 bits

	normal = getNormalVector(int((vertex.data[0] >> 20) & 0x7)); // 3 bits

	uv.x = float((vertex.data[0] >> 23) & 0x1F); // 5 bits
	uv.y = float((vertex.data[0] >> 28) & 0x1F); // 5 bits

	light = uint((vertex.data[0] >> 33) & 0xFF); // 8 bits

	ao = uint((vertex.data[0] >> 41) & 0x3); // 2 bits

	texLayer = uint(vertex.data[1] & 0xFFFFFFFF); // 32 bits
}

void extractWaterVertexData(
	in ChunkWaterVertex vertex,
	out vec3 position,
	out vec3 normal,
	out vec2 uv,
	out uint texLayer,
	out uint ao,
	out uint light
)
{
	position.x = float(vertex.data[0] & 0x1F); // 5 bits
	position.y = float((vertex.data[0] >> 5) & 0x3FF); // 10 bits
	position.z = float((vertex.data[0] >> 15) & 0x1F); // 5 bits

	normal = getNormalVector(int((vertex.data[0] >> 20) & 0x7)); // 3 bits

	uv.x = float((vertex.data[0] >> 23) & 0x1F); // 5 bits
	uv.y = float((vertex.data[0] >> 28) & 0x1F); // 5 bits

	light = uint((vertex.data[0] >> 33) & 0xFF); // 8 bits

	ao = uint((vertex.data[0] >> 41) & 0x3); // 2 bits

	texLayer = uint(vertex.data[1] & 0xFFFFFFFF); // 32 bits
}