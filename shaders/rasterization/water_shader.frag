#version 450

layout(set = 2, binding = 0) uniform sampler2DArray tex;
layout(set = 3, binding = 0) uniform sampler2D shadow_map;

layout(input_attachment_index = 0, set = 4, binding = 0) uniform subpassInput color;
layout(input_attachment_index = 1, set = 4, binding = 1) uniform subpassInput depth;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 shadow_coords;

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

float linearize_depth(float d, float n, float f)
{
    return n * f / (f + d * (n - f));
}

void main()
{
	float min_light = 0.2;

	float max_shadow_light = 0.8;
	float shadow_factor = compute_shadow_factor(shadow_coords, shadow_map, 10000, 3);

	float light = min_light + max_shadow_light * shadow_factor;

	vec4 water_texture_color = texture(tex, frag_tex_coord);
	vec3 background_color = subpassLoad(color).xyz;


	float depth_value = subpassLoad(depth).x;
	float depth_diff = depth_value - gl_FragCoord.z;

	if (depth_diff < 0.0) // if the fragment is behind the depth value
	{
		discard;
	}

	// convert the depth difference to meters (blocks)
	float depth_dist = depth_diff * 1000;

	// add water fog depending on the depth
	float fog_factor = depth_diff * 10000;

	// add fog to the background color
	background_color = mix(background_color, vec3(0.0, 0.0, 0.0), fog_factor);

	vec3 final_color = water_texture_color.a * water_texture_color.rgb + (1.0 - water_texture_color.a) * background_color;
	
	out_color = vec4(final_color, 1.0);
}
