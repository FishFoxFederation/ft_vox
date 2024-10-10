#version 450

#include "common.glsl"

layout(push_constant) uniform PushConstants
{
	ModelMatrice pc;
};

layout(location = 0) in vec3 positions;
layout(location = 2) in vec2 tex_coords;
layout(location = 3) in uint tex_layer;

layout(location = 0) out vec3 frag_tex_coords;

void main()
{
	gl_Position = pc.model * vec4(positions, 1.0);
	frag_tex_coords = vec3(tex_coords, tex_layer);
}
