#version 450

#include "common.glsl"

layout(set = 0, binding = CAMERA_MATRICES_BINDING) uniform CameraMatrices
{
	ViewProjMatrices cm;
};

layout(push_constant) uniform PushConstants
{
	GlobalPushConstant pc;
};

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

layout(location = 0) out vec2 fragTexCoords;

void main()
{
	gl_Position = cm.proj * cm.view * pc.matrice * vec4(positions, 1.0);

	fragTexCoords = texCoords;
}
