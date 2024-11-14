#version 460

#include "common.glsl"

layout(set = 0, binding = CAMERA_MATRICES_BINDING) uniform CameraMatrices
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
