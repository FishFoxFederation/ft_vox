#version 450

layout (set = 0, binding = 0) uniform sampler2D image;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	// float depth = texture(image, inUV).r;
	// outColor = vec4(1.0 - (1.0 - depth) * 100.0);

	outColor = texture(image, inUV).rrra;
}
