#version 450

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_scalar_block_layout : enable

#define SHADOW_MAP_COUNT 5
#define SHADOW_MAP_SIZE 4096

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;
layout(set = 1, binding = 0, scalar) uniform LightSpaceMatrices
{
	mat4 lightSpaceMatrices[SHADOW_MAP_COUNT];
	vec4 planeDistances[SHADOW_MAP_COUNT];
	vec3 lightDir;
	float farPlane;
};
layout(set = 2, binding = 0) uniform sampler2DArray tex;
layout(set = 3, binding = 0) uniform sampler2DArray shadow_map;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 frag_pos_world_space;
layout(location = 3) in float frag_ao;

layout(location = 0) out vec4 out_color;

int g_layer = 0;
float compute_shadow_factor(
	vec4 world_space_pos,
	sampler2DArray shadowMap,
	// uint shadow_map_size,
	uint pcf_size
)
{
	vec4 fragPosViewSpace = cm.view * world_space_pos;
	float depthValue = abs(fragPosViewSpace.z);

	int layer = -1;
	for (int i = 0; i < SHADOW_MAP_COUNT; i++)
	{
		if (depthValue < planeDistances[i].x)
		{
			layer = i;
			break;
		}
	}
	if (layer == -1)
	{
		layer = SHADOW_MAP_COUNT;
	}
	g_layer = layer;
	vec4 fragPosLightSpace = lightSpaceMatrices[layer] * world_space_pos;

	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range (Vulkan's Z is already in [0..1])
	vec2 projCoordsXY = projCoords.xy * 0.5 + 0.5;
	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if (projCoords.z > 1.0)
	{
		return 0.0;
	}

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	if (currentDepth  > 1.0)
	{
		return 0.0;
	}
	// calculate bias (based on depth map resolution and slope)
	vec3 normal = normalize(frag_normal);

	if (dot(normal, lightDir) < 0.0)
	{
		return 1.0;
	}

	// float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
	// float bias = 0.0005 * tan(acos(cosTheta));
	// bias = clamp(bias, 0.0, 0.01);

	// float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	// if (layer == SHADOW_MAP_COUNT)
	// {
	// 	bias *= 1 / (farPlane * 0.5);
	// }
	// else
	// {
	// 	bias *= 1 / (planeDistances[layer].x * 0.5);
	// }
	// bias = 0.00005;
	// currentDepth -= bias;

	// PCF
	float shadow = 0.0;
	float sampleCount = 0.0;
	vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			vec3 texCoord = vec3(projCoordsXY + vec2(x, y) * texelSize, layer);
			if (texCoord.x < 0.0 || texCoord.y < 0.0 || texCoord.x > 1.0 || texCoord.y > 1.0)
			{
				continue;
			}
			const float pcfDepth = texture(shadowMap, texCoord).r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
			sampleCount += 1.0;
		}
	}

	// 1.0 is fully in shadow, 0.0 is fully lit
	return shadow / sampleCount;

	// Compute the light space position in NDC
	// vec3 light_space_ndc = light_space_pos.xyz /= light_space_pos.w;

	// if (
	// 	abs(light_space_ndc.x) > 1.0 ||
	// 	abs(light_space_ndc.y) > 1.0 ||
	// 	abs(light_space_ndc.z) > 1.0
	// )
	// {
	//  	return 0.5;
	// }

	// // Translate from NDC to shadow map space (Vulkan's Z is already in [0..1])
	// vec2 shadow_map_coord = light_space_ndc.xy * 0.5 + 0.5;

	// // compute total number of samples to take from the shadow map
	// int pcf_size_minus_1 = int(pcf_size - 1);
	// float kernel_size = 2.0 * pcf_size_minus_1 + 1.0;
	// float num_samples = kernel_size * kernel_size;

	// // Counter for the shadow map samples not in the shadow
	// float lighted_count = 0.0;

	// // Take samples from the shadow map
	// float shadow_map_texel_size = 1.0 / shadow_map_size;
	// for (int x = -pcf_size_minus_1; x <= pcf_size_minus_1; x++)
	// {
	// 	for (int y = -pcf_size_minus_1; y <= pcf_size_minus_1; y++)
	// 	{
	// 		// Compute coordinate for this PFC sample
	// 		vec2 pcf_coord = shadow_map_coord + vec2(x, y) * shadow_map_texel_size;

	// 		// Check if the sample is in light or in the shadow
	// 		if (light_space_ndc.z <= texture(shadow_map, vec3(pcf_coord.xy, 0.0)).x)
	// 		{
	// 			lighted_count += 1.0;
	// 		}
	// 	}
	// }

	// return lighted_count / num_samples;
}

void main()
{
	float base_light = 1.0;

	float max_ao_shadow = 0.9;
	float ao_factor = frag_ao / 3.0;

	float max_shadow = 0.9;
	float shadow_factor = compute_shadow_factor(frag_pos_world_space, shadow_map, 3);

	vec3 debug_color;
	if (g_layer < 0.1) debug_color = vec3(1.0, 0.0, 0.0);
	else if (g_layer < 1.1) debug_color = vec3(0.0, 1.0, 0.0);
	else if (g_layer < 2.1) debug_color = vec3(0.0, 0.0, 1.0);
	else if (g_layer < 3.1) debug_color = vec3(1.0, 1.0, 0.0);
	else debug_color = vec3(1.0, 0.0, 1.0);

	// float light = base_light + max_shadow * shadow_factor - max_ao_shadow * ao_factor;
	// float light = base_light - max_ao_shadow * ao_factor;
	float light = base_light - max_shadow * shadow_factor;

	vec4 texture_color = texture(tex, frag_tex_coord);

	if (texture_color.a < 0.01)
	{
		discard;
	}

	out_color = vec4(mix(texture_color.rgb, debug_color, 0.0) * light, texture_color.a);
}
