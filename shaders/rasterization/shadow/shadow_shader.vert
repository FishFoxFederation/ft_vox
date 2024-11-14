#version 460

#include "common.glsl"

layout(set = 0, binding = INSTANCE_DATA_BINDING, std140) readonly buffer InstanceDataBinding
{
	InstanceData instanceData[];
} instanceDataBinding;

layout(location = 0) in u64vec2 vertexData;

layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	vec3 positions; vec3 normal; vec2 tex_coords; uint tex_layer; uint ao; uint light;
	extractBlockVertexData(vertexData, positions, normal, tex_coords, tex_layer, ao, light);

	const InstanceData instance_data = instanceDataBinding.instanceData[gl_BaseInstance];

	gl_Position = instance_data.matrice * vec4(positions, 1.0);
	frag_tex_coords = vec3(tex_coords, tex_layer);
}
