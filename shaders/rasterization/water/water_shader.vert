#version 450

#include "common.glsl"

layout(set = 0, binding = 0) uniform CameraMatrices
{
	ViewProjMatrices cm;
};

layout(push_constant) uniform PushConstants
{
	GlobalPushConstant pc;
};

layout(location = 0) in u64vec2 vertexData;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTexCoords;
layout(location = 2) out vec4 fragPosWorldSpace;

void main()
{
	vec3 positions; vec3 normal; vec2 texCoords; uint texLayer; uint ao; uint light;
	extractBlockVertexData(vertexData, positions, normal, texCoords, texLayer, ao, light);

	gl_Position = cm.proj * cm.view * pc.matrice * vec4(positions, 1.0);

	fragNormal = normal;
	fragTexCoords = vec3(texCoords, texLayer);

	fragPosWorldSpace = pc.matrice * vec4(positions, 1.0);
}
