#version 450

layout (set = 0, binding = 0) uniform sampler2DArray image;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

float depth_to_distance(float d, float n, float f)
{
	return n * f / (f + d * (n - f));
}

void main()
{
	float depth = texture(image, vec3(inUV, 1.0)).r;
	// float dist = depth_to_distance(depth, -1000, 1000) / 2000.0;

	outColor = vec4(vec3(depth), 1.0);
}
