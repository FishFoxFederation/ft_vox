#version 450

layout(set = 1, binding = 0) uniform sampler2DArray tex;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 normal = abs(normalize(fragNormal));
	float intensity = normal.x * 0.200 + normal.y * 0.500 + normal.z * 0.800;

    outColor = texture(tex, fragTexCoord) * intensity;
}
