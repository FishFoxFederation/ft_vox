#version 450

layout(set=1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;

void main() {
	outColor = texture(texSampler, fragTexCoord);

	vec3 abs_normal = abs(fragNormal);

	float grey_shade = abs_normal.x * 0.299 + abs_normal.y * 0.587 + abs_normal.z * 0.114;

	outNormal = vec3(grey_shade, grey_shade, grey_shade);
}
