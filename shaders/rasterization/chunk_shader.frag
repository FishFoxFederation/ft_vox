#version 450

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_scalar_block_layout : enable

#include "common.glsl"

layout(set = 0, binding = CAMERA_MATRICES_BINDING) uniform CameraMatrices
{
	ViewProjMatrices cm;
};
layout(set = 0, binding = BLOCK_TEXTURES_BINDING) uniform sampler2DArray tex;
layout(set = 0, binding = SHADOW_MAP_BINDING) uniform sampler2DArray shadow_map;
layout(set = 0, binding = SUN_MATRICES_BINDING) uniform LightSpaceMatrices
{
	ShadowMapLight shadow_map_light;
};

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 frag_pos_world_space;
layout(location = 3) in float frag_ao;
layout(location = 4) in float frag_sky_light;
layout(location = 5) in float frag_block_light;

layout(location = 0) out vec4 out_color;

float sample_shadow_map(vec4 world_space_pos, sampler2DArray shadowMap, int layer)
{
	vec4 fragPosLightSpace = shadow_map_light.view_proj[layer] * world_space_pos;
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range (Vulkan's Z is already in [0..1])
	vec2 uv = projCoords.xy * 0.5 + 0.5;
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

	// return full shadow if the fragment is facing away from the light
	vec3 normal = normalize(frag_normal);
	if (dot(normal, shadow_map_light.light_dir) < 0.0)
	{
		return 1.0;
	}

	// calculate texel size
	vec2 texSize = textureSize(shadowMap, 0).xy;
	vec2 texelSize = 1.0 / texSize;

	// PCF
	float shadow = 0.0;
	int range = 2;
	float count = 0.0;
	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			vec3 texCoord = vec3(uv + vec2(x, y) * texelSize, layer);
			const float pcfDepth = texture(shadowMap, texCoord).r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;

			// Bilinear filtering
			// float offset = 0.5;
			// vec2 st = texCoord.xy * texSize - 0.5;
			// vec2 i = floor(st);
			// vec2 w = fract(st);
			// float a = currentDepth > texture(shadowMap, vec3((i + vec2(-offset,-offset)) / texSize, layer)).r ? 1.0 : 0.0;
			// float b = currentDepth > texture(shadowMap, vec3((i + vec2( offset,-offset)) / texSize, layer)).r ? 1.0 : 0.0;
			// float c = currentDepth > texture(shadowMap, vec3((i + vec2(-offset, offset)) / texSize, layer)).r ? 1.0 : 0.0;
			// float d = currentDepth > texture(shadowMap, vec3((i + vec2( offset, offset)) / texSize, layer)).r ? 1.0 : 0.0;
			// shadow += mix(mix(a, b, w.x), mix(c, d, w.x), w.y);

			count += 1.0;
		}
	}

	// 1.0 is fully in shadow, 0.0 is fully lit
	return shadow / count;
}

float compute_shadow_factor(
	vec4 world_space_pos,
	sampler2DArray shadowMap,
	uint pcf_size
)
{
	vec4 fragPosViewSpace = cm.view * world_space_pos;
	float depthValue = abs(fragPosViewSpace.z);

	int layer = -1;
	float blendFactor = 0.0;
	bool blend = false;
	for (int i = 0; i < SHADOW_MAP_MAX_COUNT; i++)
	{
		if (depthValue < shadow_map_light.plane_distances[i].x)
		{
			layer = i;
			if (i < SHADOW_MAP_MAX_COUNT - 1 && depthValue > shadow_map_light.plane_distances[i].x - shadow_map_light.blend_distance)
			{
				blendFactor = (shadow_map_light.blend_distance + depthValue - shadow_map_light.plane_distances[i].x) / shadow_map_light.blend_distance;
				blend = true;
			}
			break;
		}
	}
	if (layer == -1)
	{
		layer = SHADOW_MAP_MAX_COUNT;
	}

	if (blend)
	{
		float shadow1 = sample_shadow_map(world_space_pos, shadowMap, layer);
		float shadow2 = sample_shadow_map(world_space_pos, shadowMap, layer + 1);
		return mix(shadow1, shadow2, blendFactor);
	}

	return sample_shadow_map(world_space_pos, shadowMap, layer);
}

vec3 debugColor(float value)
{
	if (value < 0.01) return vec3(1.0, 0.0, 0.0);
	if (value < 1.01) return vec3(0.0, 1.0, 0.0);
	if (value < 2.01) return vec3(0.0, 0.0, 1.0);
	if (value < 3.01) return vec3(0.0, 1.0, 1.0);
	if (value < 4.01) return vec3(1.0, 1.0, 0.0);
	if (value < 5.01) return vec3(1.0, 0.0, 1.0);
	return vec3(0.5, 0.5, 0.5);
}

void main()
{

	float max_ao = 0.8;
	float ao_factor = 1.0 - (frag_ao / 3.0 * max_ao);

	float max_shadow = 0.7;
	// float shadow_factor = 1.0 - compute_shadow_factor(frag_pos_world_space, shadow_map, 3) * max_shadow;
	float shadow_factor = compute_shadow_factor(frag_pos_world_space, shadow_map, 3) * max_shadow;

	float sky_light = frag_sky_light / 15.0 - shadow_factor;
	float block_light = frag_block_light / 15.0;
	float max_base_light = 0.8;
	float base_light = max(sky_light, block_light);

	const float min_light = 0.05;
	float light = min_light + base_light * (1.0 - min_light);
	light = min_light + light * ao_factor * (1.0 - min_light);
	// light = min_light + light * shadow_factor * (1.0 - min_light);

	vec4 texture_color = texture(tex, frag_tex_coord);

	if (texture_color.a < 0.01)
	{
		discard;
	}

	out_color = vec4(texture_color.rgb * light, texture_color.a);
}
