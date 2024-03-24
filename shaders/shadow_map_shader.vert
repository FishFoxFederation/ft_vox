#version 450

layout(push_constant) uniform PushConstants
{
	mat4 view;
	mat4 projection;
	mat4 model;
}pc;

layout(location = 0) in ivec3 positions;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in uint texLayer;

layout(location = 0) out vec3 position;

void main()
{
    gl_Position = pc.projection * pc.view * pc.model * vec4(positions, 1.0);
	position = gl_Position.xyz;
}
