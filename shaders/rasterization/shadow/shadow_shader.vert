#version 460

#include "common.glsl"

layout(set = 0, binding = INSTANCE_DATA_BINDING, std140) readonly buffer InstanceDataBinding
{
	InstanceData instanceData[];
} instanceDataBinding;


layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	InstanceData instance_data = instanceDataBinding.instanceData[gl_BaseInstance];

	ChunkBlockVertex vertex_data = instance_data.block_vextex_buffer.vertices[gl_VertexIndex];

	vec3 positions; vec3 normal; vec2 tex_coords; uint tex_layer; uint ao; uint light;
	extractBlockVertexData(vertex_data, positions, normal, tex_coords, tex_layer, ao, light);


	gl_Position = instance_data.matrice * vec4(positions, 1.0);
	frag_tex_coords = vec3(tex_coords, tex_layer);
}
