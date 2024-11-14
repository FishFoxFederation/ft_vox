#version 460

#include "common.glsl"

layout (set = 0, binding = TEST_IMAGE_BINDING) uniform sampler2DArray image;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

float depth_to_distance(float d, float n, float f)
{
	return n * f / (f + d * (n - f));
}

void main()
{
	int layer = 0;
	vec2  fres = textureSize(image, 0).xy;
	vec2 st = inUV*fres - 0.5;
	vec2 i = floor(st);
	vec2 w = fract(st);
	float a = texture(image, vec3((i+vec2(-0.5,-0.5))/fres, layer)).r;
	float b = texture(image, vec3((i+vec2(0.5, -0.5))/fres, layer)).r;
	float c = texture(image, vec3((i+vec2(-0.5, 0.5))/fres, layer)).r;
	float d = texture(image, vec3((i+vec2(0.5,  0.5))/fres, layer)).r;
	float depth = mix(mix(a, b, w.x), mix(c, d, w.x), w.y);

	outColor = vec4(vec3(depth), 1.0);
}
