#pragma once

#include "define.hpp"
#include "logger.hpp"
#include "Transform.hpp"

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include <vector>

class PlayerModel
{

public:

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};

	PlayerModel() {}
	~PlayerModel() {}

	PlayerModel(PlayerModel & other) = delete;
	PlayerModel(PlayerModel && other) = delete;
	PlayerModel & operator=(PlayerModel & other) = delete;
	PlayerModel & operator=(PlayerModel && other) = delete;

	constexpr static inline double player_height = 1.8;
	constexpr static inline double unit = player_height / 32.0;

	constexpr static inline glm::dvec3 chest_size = glm::dvec3(8.0, 12.0, 4.0) * unit;
	constexpr static inline glm::dvec3 head_size  = glm::dvec3(8.0,  8.0, 8.0) * unit;
	constexpr static inline glm::dvec3 leg_size   = glm::dvec3(4.0, 12.0, 4.0) * unit;
	constexpr static inline glm::dvec3 arm_size   = glm::dvec3(4.0, 12.0, 4.0) * unit;

	// static inline glm::dvec3 chest_size = glm::dvec3(0.45, 0.675, 0.225);
	// static inline glm::dvec3 head_size  = glm::dvec3(0.45,  0.45, 0.45);
	// static inline glm::dvec3 leg_size   = glm::dvec3(0.225, 0.675, 0.225);
	// static inline glm::dvec3 arm_size   = glm::dvec3(0.225, 0.675, 0.225);

	constexpr static inline glm::vec3 chest_pos     = glm::vec3{0.0, leg_size.y, 0.0};
	constexpr static inline glm::vec3 head_pos      = glm::vec3{0.0, chest_size.y, 0.0};
	constexpr static inline glm::vec3 left_leg_pos  = glm::vec3{-leg_size.x / 2, 0.0, 0.0};
	constexpr static inline glm::vec3 right_leg_pos = glm::vec3{leg_size.x / 2, 0.0, 0.0};
	constexpr static inline glm::vec3 left_arm_pos  = glm::vec3{-(arm_size.x / 2 + chest_size.x / 2), 0.0, 0.0};
	constexpr static inline glm::vec3 right_arm_pos = glm::vec3{(arm_size.x / 2 + chest_size.x / 2), 0.0, 0.0};

};
