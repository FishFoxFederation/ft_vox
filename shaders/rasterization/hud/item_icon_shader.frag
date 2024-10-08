#version 450

layout (set = 0, binding = 0) uniform sampler2DArray image;

layout(push_constant) uniform PushConstants
{
	int index;
} pc;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	outColor = texture(image, vec3(inUV, pc.index));
}
