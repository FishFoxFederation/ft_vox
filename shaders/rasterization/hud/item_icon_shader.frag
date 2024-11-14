#version 460

#include "common.glsl"

layout (set = 0, binding = ITEM_ICON_TEXTURE_BINDING) uniform sampler2DArray image;

layout(push_constant) uniform PushConstants
{
	GlobalPushConstant pc;
};

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(image, vec3(inUV, pc.layer));
}
