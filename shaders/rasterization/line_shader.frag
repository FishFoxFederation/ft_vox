#version 460

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	GlobalPushConstant pc;
};

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = pc.color;
}
