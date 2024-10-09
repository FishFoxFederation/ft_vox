#version 450

#include "common.glsl"

layout(set = 0, binding = 0) uniform CameraMatrices
{
	ViewProjMatrices cm;
};

layout(push_constant) uniform PushConstants
{
	ModelMatrice pc;
};

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;

layout(location = 0) out vec3 fragColors;

void main()
{
    gl_Position = cm.proj * cm.view * pc.model * vec4(positions, 1.0);
    fragColors = normals;
}
