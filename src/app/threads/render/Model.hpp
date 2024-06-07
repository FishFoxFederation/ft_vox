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
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			return attributeDescriptions;
		}
	};

	enum class Part
	{
		CHEST,
		HEAD,
		LEG,
		ARM
	};

	PlayerModel() {}
	~PlayerModel() {}

	PlayerModel(PlayerModel & other) = delete;
	PlayerModel(PlayerModel && other) = delete;
	PlayerModel & operator=(PlayerModel & other) = delete;
	PlayerModel & operator=(PlayerModel && other) = delete;

	static void getGeometry(
		std::vector<Vertex> & vertices,
		std::vector<uint32_t> & indices,
		const Part part
	)
	{
		switch (part)
		{
			case Part::CHEST:
				geometry(vertices, indices, chest_size);
				break;
			case Part::HEAD:
				geometry(vertices, indices, head_size);
				break;
			case Part::LEG:
				geometry(vertices, indices, leg_size);
				break;
			case Part::ARM:
				geometry(vertices, indices, arm_size);
				break;
			default:
				LOG_ERROR("Unknown player model part");
				break;
		}
	}

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
	constexpr static inline glm::vec3 left_leg_pos  = glm::vec3{-leg_size.x / 2, -leg_size.y, 0.0};
	constexpr static inline glm::vec3 right_leg_pos = glm::vec3{leg_size.x / 2, -leg_size.y, 0.0};
	constexpr static inline glm::vec3 left_arm_pos  = glm::vec3{-(arm_size.x / 2 + chest_size.x / 2), 0.0, 0.0};
	constexpr static inline glm::vec3 right_arm_pos = glm::vec3{(arm_size.x / 2 + chest_size.x / 2), 0.0, 0.0};

private:

	/**
	 * @brief Create the geometry for a cube with the given size and with th origin at the bottom center
	 *
	 */
	static void geometry(
		std::vector<Vertex> & vertices,
		std::vector<uint32_t> & indices,
		const glm::vec3 & size
	)
	{
		const glm::vec3 s = size / 2.0f;
		vertices = {
			// right
			{{s.x, size.y,  s.z}, {1.0f, 0.0f, 0.0f}},
			{{s.x, size.y, -s.z}, {1.0f, 0.0f, 0.0f}},
			{{s.x,   0.0f, -s.z}, {1.0f, 0.0f, 0.0f}},
			{{s.x,   0.0f,  s.z}, {1.0f, 0.0f, 0.0f}},

			// left
			{{-s.x, size.y,  s.z}, {-1.0f, 0.0f, 0.0f}},
			{{-s.x, size.y, -s.z}, {-1.0f, 0.0f, 0.0f}},
			{{-s.x,   0.0f, -s.z}, {-1.0f, 0.0f, 0.0f}},
			{{-s.x,   0.0f,  s.z}, {-1.0f, 0.0f, 0.0f}},

			// top
			{{ s.x, size.y,  s.z}, {0.0f, 1.0f, 0.0f}},
			{{ s.x, size.y, -s.z}, {0.0f, 1.0f, 0.0f}},
			{{-s.x, size.y, -s.z}, {0.0f, 1.0f, 0.0f}},
			{{-s.x, size.y,  s.z}, {0.0f, 1.0f, 0.0f}},

			// bottom
			{{ s.x, 0.0f,  s.z}, {0.0f, -1.0f, 0.0f}},
			{{ s.x, 0.0f, -s.z}, {0.0f, -1.0f, 0.0f}},
			{{-s.x, 0.0f, -s.z}, {0.0f, -1.0f, 0.0f}},
			{{-s.x, 0.0f,  s.z}, {0.0f, -1.0f, 0.0f}},

			// front
			{{ s.x, size.y, s.z}, {0.0f, 0.0f, 1.0f}},
			{{ s.x,   0.0f, s.z}, {0.0f, 0.0f, 1.0f}},
			{{-s.x,   0.0f, s.z}, {0.0f, 0.0f, 1.0f}},
			{{-s.x, size.y, s.z}, {0.0f, 0.0f, 1.0f}},

			// back
			{{ s.x, size.y, -s.z}, {0.0f, 0.0f, -1.0f}},
			{{ s.x,   0.0f, -s.z}, {0.0f, 0.0f, -1.0f}},
			{{-s.x,   0.0f, -s.z}, {0.0f, 0.0f, -1.0f}},
			{{-s.x, size.y, -s.z}, {0.0f, 0.0f, -1.0f}}
		};

		indices = {
			0, 2, 1, 0, 3, 2,
			4, 5, 6, 6, 7, 4,
			8, 9, 10, 10, 11, 8,
			12, 14, 13, 12, 15, 14,
			16, 18, 17, 16, 19, 18,
			20, 21, 22, 22, 23, 20
		};
	}

};
