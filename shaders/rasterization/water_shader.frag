#version 450

#extension GL_EXT_scalar_block_layout : enable

#define SHADOW_MAP_COUNT 8

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
	float blendDistance;
};
layout(set = 2, binding = 0) uniform sampler2DArray tex;
layout(set = 3, binding = 0) uniform sampler2DArray shadow_map;

layout(input_attachment_index = 0, set = 4, binding = 0) uniform subpassInput color;
layout(input_attachment_index = 1, set = 4, binding = 1) uniform subpassInput depth;

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 frag_pos_world_space;

layout(location = 0) out vec4 out_color;


float sample_shadow_map(vec4 world_space_pos, sampler2DArray shadowMap, int layer)
{
	vec4 fragPosLightSpace = lightSpaceMatrices[layer] * world_space_pos;
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
	if (dot(normal, lightDir) < 0.0)
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
	for (int i = 0; i < SHADOW_MAP_COUNT; i++)
	{
		if (depthValue < planeDistances[i].x)
		{
			layer = i;
			if (i < SHADOW_MAP_COUNT - 1 && depthValue > planeDistances[i].x - blendDistance)
			{
				blendFactor = (blendDistance + depthValue - planeDistances[i].x) / blendDistance;
				blend = true;
			}
			break;
		}
	}
	if (layer == -1)
	{
		layer = SHADOW_MAP_COUNT;
	}

	if (blend)
	{
		float shadow1 = sample_shadow_map(world_space_pos, shadowMap, layer);
		float shadow2 = sample_shadow_map(world_space_pos, shadowMap, layer + 1);
		return mix(shadow1, shadow2, blendFactor);
	}

	return sample_shadow_map(world_space_pos, shadowMap, layer);
}

float depth_to_distance(float d, float n, float f)
{
	return n * f / (f + d * (n - f));
}

void main()
{
	float min_light = 0.2;

	float max_shadow_light = 0.8;
	float shadow_factor = compute_shadow_factor(frag_pos_world_space, shadow_map, 3);

	float light = min_light + max_shadow_light * shadow_factor;

	vec4 water_texture_color = texture(tex, frag_tex_coord);
	vec3 background_color = subpassLoad(color).xyz;


	float z_depth = subpassLoad(depth).x;
	float background_depth = depth_to_distance(z_depth, 0.01, 1000.0) / 1000.0;
	float water_depth = depth_to_distance(gl_FragCoord.z, 0.01, 1000.0) / 1000.0;
	float depth_diff = background_depth - water_depth;

	// if the fragment is behind the depth value
	if (depth_diff < 0.0)
	{
		discard;
	}

	// water fog depending on the depth
	float fog_factor = depth_diff * 100.0;

	// add fog to the background color
	// background_color = mix(background_color, vec3(0.0, 0.0, 0.0), fog_factor);
	background_color *= 1.0 - fog_factor;

	// mix the water color with the background color (alpha blending)
	vec3 final_color = water_texture_color.a * water_texture_color.rgb + (1.0 - water_texture_color.a) * background_color;

	out_color = vec4(final_color, 1.0);
}
