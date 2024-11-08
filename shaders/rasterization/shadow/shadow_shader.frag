#version 450

#include "common.glsl"

layout(set = 0, binding = BLOCK_TEXTURES_BINDING) uniform sampler2DArray block_textures;

layout(location = 0) in vec3 frag_tex_coords;

void main()
{
	vec4 color = texture(block_textures, frag_tex_coords);

	if (color.a < 0.01)
		discard;
}
