#version 450

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	ObjectData obj_data;
};

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = obj_data.color;
}
