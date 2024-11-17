#version 460

#include "common.glsl"

layout(set = 0, binding = CAMERA_MATRICES_BINDING) uniform CameraMatrices
{
	ViewProjMatrices cm;
};
layout(set = 0, binding = INSTANCE_DATA_BINDING, std140) readonly buffer InstanceDataBinding
{
	InstanceData instanceData[];
} instanceDataBinding;


layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_tex_coords;
layout(location = 2) out vec4 frag_pos_world_space;

void main()
{
	InstanceData instance_data = instanceDataBinding.instanceData[gl_BaseInstance];

	ChunkWaterVertex vertex_data = instance_data.water_vertex_buffer.vertices[gl_VertexIndex];

	vec3 positions; vec3 normal; vec2 tex_coords; uint tex_layer; uint ao; uint light;
	extractWaterVertexData(vertex_data, positions, normal, tex_coords, tex_layer, ao, light);


	gl_Position = cm.proj * cm.view * instance_data.matrice * vec4(positions, 1.0);

	frag_normal = normal;
	frag_tex_coords = vec3(tex_coords, tex_layer);

	frag_pos_world_space = instance_data.matrice * vec4(positions, 1.0);
}
