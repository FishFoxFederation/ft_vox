#version 450

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_scalar_block_layout : enable

#define SHADOW_MAP_COUNT 5
// #define SHADOW_MAP_SIZE 4096

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

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_tex_coord;
layout(location = 2) in vec4 frag_pos_world_space;
layout(location = 3) in float frag_ao;

layout(location = 0) out vec4 out_color;

vec3 debugColor(int layer)
{
	if (layer == 0) return vec3(1.0, 0.0, 0.0);
	if (layer == 1) return vec3(0.0, 1.0, 0.0);
	if (layer == 2) return vec3(0.0, 0.0, 1.0);
	if (layer == 3) return vec3(1.0, 1.0, 0.0);
	if (layer == 4) return vec3(1.0, 0.0, 1.0);
	return vec3(0.0, 1.0, 1.0);
}

vec4 textureGood( sampler2DArray sam, vec2 uv, int layer )
{
	ivec2 ires = textureSize( sam, 0 ).xy;
	vec2  fres = vec2( ires );

	vec2 st = uv*fres - 0.5;
	vec2 i = floor( st );
	vec2 w = fract( st );

	vec4 a = texture( sam, vec3((i+vec2(0.5,0.5))/fres, layer) );
	vec4 b = texture( sam, vec3((i+vec2(1.5,0.5))/fres, layer) );
	vec4 c = texture( sam, vec3((i+vec2(0.5,1.5))/fres, layer) );
	vec4 d = texture( sam, vec3((i+vec2(1.5,1.5))/fres, layer) );

	return mix(mix(a, b, w.x), mix(c, d, w.x), w.y);
}

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

int g_layer = 0;
float g_blendFactor = 0.0;

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
				g_blendFactor = blendFactor;
				blend = true;
			}
			break;
		}
	}
	if (layer == -1)
	{
		layer = SHADOW_MAP_COUNT;
	}
	g_layer = layer;

	if (blend)
	{
		float shadow1 = sample_shadow_map(world_space_pos, shadowMap, layer);
		float shadow2 = sample_shadow_map(world_space_pos, shadowMap, layer + 1);
		return mix(shadow1, shadow2, blendFactor);
	}

	return sample_shadow_map(world_space_pos, shadowMap, layer);
}

void main()
{
	float base_light = 1.0;

	float max_ao_shadow = 0.9;
	float ao_factor = frag_ao / 3.0;

	float max_shadow = 0.9;
	float shadow_factor = compute_shadow_factor(frag_pos_world_space, shadow_map, 3);

	vec3 debug_color = mix(debugColor(g_layer), debugColor(g_layer + 1), g_blendFactor);

	// float light = base_light - max_shadow * shadow_factor - max_ao_shadow * ao_factor;
	// float light = base_light - max_ao_shadow * ao_factor;
	float light = base_light - max_shadow * shadow_factor;

	vec4 texture_color = texture(tex, frag_tex_coord);

	if (texture_color.a < 0.01)
	{
		discard;
	}

	out_color = vec4(mix(texture_color.rgb, debug_color, 0.0) * light, texture_color.a);
}
