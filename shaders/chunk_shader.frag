#version 450

layout(set = 2, binding = 0) uniform sampler2DArray tex;
layout(set = 3, binding = 0) uniform sampler2D shadow_map;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 shadow_coords;
layout(location = 3) in float fragAO;

layout(location = 0) out vec4 out_color;


float compute_shadow_factor(
	vec4 light_space_pos,
	sampler2D shadow_map,
	uint shadow_map_size,
	uint pcf_size
)
{
	// Compute the light space position in NDC
	vec3 light_space_ndc = light_space_pos.xyz /= light_space_pos.w;

	if (
		abs(light_space_ndc.x) > 1.0 ||
		abs(light_space_ndc.y) > 1.0 ||
		abs(light_space_ndc.z) > 1.0
	)
	{
	 	return 0.5;
	}

	// Translate from NDC to shadow map space (Vulkan's Z is already in [0..1])
	vec2 shadow_map_coord = light_space_ndc.xy * 0.5 + 0.5;

	// compute total number of samples to take from the shadow map
	int pcf_size_minus_1 = int(pcf_size - 1);
	float kernel_size = 2.0 * pcf_size_minus_1 + 1.0;
	float num_samples = kernel_size * kernel_size;

	// Counter for the shadow map samples not in the shadow
	float lighted_count = 0.0;

	// Take samples from the shadow map
	float shadow_map_texel_size = 1.0 / shadow_map_size;
	for (int x = -pcf_size_minus_1; x <= pcf_size_minus_1; x++)
	{
		for (int y = -pcf_size_minus_1; y <= pcf_size_minus_1; y++)
		{
			// Compute coordinate for this PFC sample
			vec2 pcf_coord = shadow_map_coord + vec2(x, y) * shadow_map_texel_size;

			// Check if the sample is in light or in the shadow
			if (light_space_ndc.z <= texture(shadow_map, pcf_coord.xy).x)
			{
				lighted_count += 1.0;
			}
		}
	}

	return lighted_count / num_samples;
}

void main()
{
	float min_light = 0.2;

	float max_ao_light = 0.5;
	float ao_factor = (1 - (fragAO / 3.0));

	float max_shadow_light = 0.5;
	// float shadow_factor = compute_shadow_factor(shadow_coords, shadow_map, 2048, 2);
	float shadow_factor = 0.0;;

	float light = min_light
				+ max_ao_light * ao_factor
				+ max_shadow_light * shadow_factor;

	out_color = texture(tex, frag_tex_coord) * light;
}
