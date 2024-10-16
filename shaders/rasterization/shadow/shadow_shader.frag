#version 450

layout(set = 2, binding = 0) uniform sampler2DArray block_textures;

layout(location = 0) in vec3 frag_tex_coords;

void main()
{
	vec4 color = texture(block_textures, frag_tex_coords);

	if (color.a < 0.01)
		discard;
}
