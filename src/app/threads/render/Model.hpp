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

	static glm::mat4 chestModel() { return m_chest.model(); }
	static glm::mat4 headModel() { return m_chest.model() * m_head.model(); }
	static glm::mat4 leftLegModel() { return m_chest.model() * m_left_leg.model(); }
	static glm::mat4 rightLegModel() { return m_chest.model() * m_right_leg.model(); }
	static glm::mat4 leftArmModel() { return m_chest.model() * m_left_arm.model(); }
	static glm::mat4 rightArmModel() { return m_chest.model() * m_right_arm.model(); }

	static void getGeometry(
		std::vector<Vertex> & vertices,
		std::vector<uint32_t> & indices,
		const Part part
	)
	{
		switch (part)
		{
			case Part::CHEST:
				geometry(vertices, indices, m_chest_size);
				break;
			case Part::HEAD:
				geometry(vertices, indices, m_head_size);
				break;
			case Part::LEG:
				geometry(vertices, indices, m_leg_size);
				break;
			case Part::ARM:
				geometry(vertices, indices, m_arm_size);
				break;
			default:
				LOG_ERROR("Unknown player model part");
				break;
		}
	}

private:

	constexpr static inline double m_player_height = 1.8;
	constexpr static inline double m_unit = m_player_height / 32.0;

	constexpr static inline glm::dvec3 m_chest_size = glm::dvec3(8.0, 12.0, 4.0) * m_unit;
	constexpr static inline glm::dvec3 m_head_size  = glm::dvec3(8.0,  8.0, 8.0) * m_unit;
	constexpr static inline glm::dvec3 m_leg_size   = glm::dvec3(4.0, 12.0, 4.0) * m_unit;
	constexpr static inline glm::dvec3 m_arm_size   = glm::dvec3(4.0, 12.0, 4.0) * m_unit;

	// static inline glm::dvec3 m_chest_size = glm::dvec3(0.45, 0.675, 0.225);
	// static inline glm::dvec3 m_head_size  = glm::dvec3(0.45,  0.45, 0.45);
	// static inline glm::dvec3 m_leg_size   = glm::dvec3(0.225, 0.675, 0.225);
	// static inline glm::dvec3 m_arm_size   = glm::dvec3(0.225, 0.675, 0.225);

	static inline Transform m_chest = Transform({0.0, m_leg_size.y, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	static inline Transform m_head = Transform({0.0, m_chest_size.y, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	static inline Transform m_left_leg = Transform({-m_leg_size.x / 2, -m_leg_size.y, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	static inline Transform m_right_leg = Transform({m_leg_size.x / 2, -m_leg_size.y, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	static inline Transform m_left_arm = Transform({-(m_arm_size.x / 2 + m_chest_size.x / 2), 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	static inline Transform m_right_arm = Transform({(m_arm_size.x / 2 + m_chest_size.x / 2), 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});

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
