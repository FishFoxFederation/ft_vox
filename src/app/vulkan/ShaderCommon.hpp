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

	// other types
	using uint = uint32_t;

#endif

#define SHADOW_MAP_MAX_COUNT 8

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

struct ObjectData
{
	mat4 matrix;
	vec4 color;
	int layer;
};


// for bindless descriptor

#define BINDLESS_DESCRIPTOR_MAX_COUNT 1024

#define BINDLESS_DESCRIPTOR_SET 0

#define BINDLESS_PARAMS_BINDING 0
#define BINDLESS_UNIFORM_BUFFER_BINDING 1
#define BINDLESS_STORAGE_BUFFER_BINDING 2
#define BINDLESS_COMBINED_IMAGE_SAMPLER_BINDING 3

struct BindlessDescriptorParams
{
	uint camera_ubo_index;
	uint pad0;
	uint pad1;
	uint pad2;
};



#endif // SHADER_COMMUN_HPP
