#version 460

#include "common.glsl"

layout (set = 0, binding = PLAYER_SKIN_BINDING) uniform sampler2D image;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(image, inUV);
}
