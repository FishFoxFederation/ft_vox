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

#define CAMERA_MATRICES_BINDING 0
#define BLOCK_TEXTURES_BINDING 1
#define SKYBOX_CUBE_MAP_BINDING 2
#define SHADOW_MAP_BINDING 3
#define WATER_RENDERPASS_INPUT_COLOR_ATTACH_BINDING 4
#define WATER_RENDERPASS_INPUT_DEPTH_ATTACH_BINDING 5
#define TEST_IMAGE_BINDING 6
#define PLAYER_SKIN_BINDING 7
#define SUN_MATRICES_BINDING 8
#define ATMOSPHERE_PARAM_BINDING 9
#define ITEM_ICON_TEXTURE_BINDING 10
#define INSTANCE_DATA_BINDING 11

struct GlobalPushConstant
{
	mat4 matrice;
	vec4 color;
	int layer;
};

struct InstanceData
{
	mat4 matrice;
	vec4 color;
	// int layer;
};

struct ViewProjMatrices
{
	mat4 view;
	mat4 proj;
};

struct ShadowMapLight
{
	mat4 view_proj[SHADOW_MAP_MAX_COUNT];
	// TODO: this is vec4 because of alignment, but it should be float
	vec4 plane_distances[SHADOW_MAP_MAX_COUNT];
	vec3 light_dir;
	float blend_distance;
};


#endif // SHADER_COMMUN_HPP
