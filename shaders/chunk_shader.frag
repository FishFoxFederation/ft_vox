#version 450

layout(set = 2, binding = 0) uniform sampler2DArray tex;
layout(set = 3, binding = 0) uniform sampler2D shadow_map;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 shadow_coords;

layout(location = 0) out vec4 outColor;


float compute_shadow_factor(vec4 light_space_pos, sampler2D shadow_map)
{
	// Convert light space position to NDC
	vec3 light_space_ndc = light_space_pos.xyz /= light_space_pos.w;

	// If the fragment is outside the light's projection then it is outside
	// the light's influence, which means it is in the shadow (notice that
	// such sample would be outside the shadow map image)
	if (abs(light_space_ndc.x) > 1.0 ||
		 abs(light_space_ndc.y) > 1.0 ||
		 abs(light_space_ndc.z) > 1.0)
		return 0.0;

	// Translate from NDC to shadow map space (Vulkan's Z is already in [0..1])
	vec2 shadow_map_coord = light_space_ndc.xy * 0.5 + 0.5;

	// Check if the sample is in the light or in the shadow
	if (light_space_ndc.z > texture(shadow_map, shadow_map_coord.xy).x)
		return 0.0; // In the shadow

	// In the light
	return 1.0;
}

void main()
{
	vec3 normal = abs(normalize(frag_normal));
	float intensity = normal.x * 0.200 + normal.y * 0.500 + normal.z * 0.800;

	vec4 texel = texture(tex, frag_tex_coord);

	intensity = 1.0;
	if (compute_shadow_factor(shadow_coords, shadow_map) == 0.0)
	{
		intensity = 0.5;
	}

	outColor = texel * intensity;
}
