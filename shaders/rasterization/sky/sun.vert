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

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 rayDir;

void main()
{
	vec4 pos = cm.proj * cm.view * pc.matrice * vec4(position, 1.0);
	gl_Position = pos.xyww;

	rayDir = normalize(position);
}
