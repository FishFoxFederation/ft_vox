#version 450

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	ObjectData obj_data;
};

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in uint texLayer;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTexCoords;

void main()
{
	gl_Position = obj_data.matrix * vec4(positions, 1.0);

	fragNormal = normal;
	fragTexCoords = vec3(texCoords, texLayer);
}
