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


layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTexCoords;
layout(location = 2) out vec4 fragPosWorldSpace;
layout(location = 3) out float fragAO;
layout(location = 4) out float fragSkyLight;
layout(location = 5) out float fragBlockLight;

void main()
{
	InstanceData instance_data = instanceDataBinding.instanceData[gl_BaseInstance];

	ChunkBlockVertex vertex_data = instance_data.block_vextex_buffer.vertices[gl_VertexIndex];

	vec3 positions; vec3 normal; vec2 texCoords; uint texLayer; uint ao; uint light;
	extractBlockVertexData2(vertex_data, positions, normal, texCoords, texLayer, ao, light);


	gl_Position = cm.proj * cm.view * instance_data.matrice * vec4(positions, 1.0);

	fragNormal = normal;
	fragTexCoords = vec3(texCoords, texLayer);

	fragPosWorldSpace = instance_data.matrice * vec4(positions, 1.0);

	fragAO = float(ao);
	fragSkyLight = float(light & 0x0F);
	fragBlockLight = float(light >> 4 & 0x0F);
}
