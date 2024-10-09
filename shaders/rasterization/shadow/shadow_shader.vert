#version 450

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	ModelMatrice pc;
};

layout(location = 0) in vec3 positions;

void main()
{
	gl_Position = pc.model * vec4(positions, 1.0);
}
