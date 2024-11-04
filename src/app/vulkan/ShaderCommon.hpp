#ifndef SHADER_COMMUN_HPP
#define SHADER_COMMUN_HPP

#ifdef __cplusplus

	#include <glm/glm.hpp>
	// Glsl types
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using ivec2 = glm::ivec2;
	using ivec3 = glm::ivec3;
	using ivec4 = glm::ivec4;
	using mat2 = glm::mat2;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;

#endif

#define SHADOW_MAP_MAX_COUNT 8

struct ViewProjMatrices
{
	mat4 view;
	mat4 proj;
};

struct ModelMatrice
{
	mat4 model;
};

struct EntityMatrices
{
	mat4 model;
	vec4 color;
};

struct ShadowMapLight
{
	mat4 view_proj[SHADOW_MAP_MAX_COUNT];
	// TODO: this is vec4 because of alignment, but it should be float
	vec4 plane_distances[SHADOW_MAP_MAX_COUNT];
	vec3 light_dir;
	float blend_distance;
};

struct PreRenderItemIconPushConstant
{
	mat4 MVP;
	int layer;
};

struct ItemIconPushConstant
{
	int layer;
};

struct LinePipelinePushConstant
{
	mat4 model;
	vec4 color;
};



#endif // SHADER_COMMUN_HPP
