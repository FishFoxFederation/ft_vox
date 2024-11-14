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

layout(location = 0) in u64vec2 vertexData;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTexCoords;
layout(location = 2) out vec4 fragPosWorldSpace;

void main()
{
	vec3 positions; vec3 normal; vec2 texCoords; uint texLayer; uint ao; uint light;
	extractBlockVertexData(vertexData, positions, normal, texCoords, texLayer, ao, light);

	const InstanceData instance_data = instanceDataBinding.instanceData[gl_BaseInstance];

	gl_Position = cm.proj * cm.view * instance_data.matrice * vec4(positions, 1.0);

	fragNormal = normal;
	fragTexCoords = vec3(texCoords, texLayer);

	fragPosWorldSpace = instance_data.matrice * vec4(positions, 1.0);
}
