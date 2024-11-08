#version 450

#include "common.glsl"

layout(set = 0, binding = BLOCK_TEXTURES_BINDING) uniform sampler2DArray block_textures;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main()
{
	const vec3 normal = abs(frag_normal);
	float light = 0.3 * normal.x + 0.9 * normal.y + 0.6 * normal.z;
	vec4 texture_color = texture(block_textures, frag_tex_coord);

	if (texture_color.a < 0.01)
	{
		discard;
	}

	out_color = vec4(texture_color.rgb * light, texture_color.a);
}
