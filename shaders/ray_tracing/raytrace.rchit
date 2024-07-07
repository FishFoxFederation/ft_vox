#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
// #extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct ObjectData
{
	uint64_t vertexBuffer;
	uint64_t indexBuffer;
};

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 uv;
};

layout(buffer_reference, scalar) buffer VertexBuffer { Vertex vertices[]; };
layout(buffer_reference, scalar) buffer IndexBuffer { ivec3 indices[]; };

layout(binding = 0, set = 2, scalar) buffer ObjectBuffer { ObjectData objects[]; };

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

void main()
{
	ObjectData object = objects[gl_InstanceCustomIndexEXT];

	IndexBuffer indexBuffer = IndexBuffer(object.indexBuffer);
	VertexBuffer vertexBuffer = VertexBuffer(object.vertexBuffer);

	ivec3 index = indexBuffer.indices[gl_PrimitiveID];

	Vertex v0 = vertexBuffer.vertices[index.x];
	Vertex v1 = vertexBuffer.vertices[index.y];
	Vertex v2 = vertexBuffer.vertices[index.z];

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the coordinates of the hit position
	const vec3 pos      = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	const vec3 world_pos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

	// Computing the normal at hit position
	const vec3 normal      = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space

	hitValue = world_normal;
}
