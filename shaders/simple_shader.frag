#version 450

layout(set = 1, binding = 0) uniform sampler2DArray tex;

layout(set = 2, binding = 0) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragTexCoord;
layout(location = 2) in vec3 shadowCoords;
layout(location = 3) in vec4 pos;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor_copy;

void main()
{
	vec3 normal = abs(normalize(fragNormal));
	float intensity = normal.x * 0.200 + normal.y * 0.500 + normal.z * 0.800;

    outColor = texture(tex, fragTexCoord) * intensity;
	outColor_copy = texture(tex, fragTexCoord) * intensity;

	// float visibility = 1.0;
	// if (texture(shadowMap, shadowCoords.xy).z < shadowCoords.z)
	// {
	// 	visibility = 0.5;
	// }

	// outColor = texture(tex, fragTexCoord) * visibility;
	// float shadow = texture(shadowMap, shadowCoords.xy).z;
	// outColor = vec4(shadow, shadow, shadow, 1.0);

}
