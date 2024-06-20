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

	class WalkAnimation
	{

	public:

		WalkAnimation() = default;
		~WalkAnimation() = default;

		void start()
		{
			m_is_active = true;
			m_start_time = std::chrono::steady_clock::now().time_since_epoch();
			m_should_stop = false;
			m_direction = 1.0;
		}

		void stop()
		{
			m_should_stop = true;
		}

		double time_since_start()
		{
			return static_cast<double>((std::chrono::steady_clock::now().time_since_epoch() - m_start_time).count()) / 1e9;
		}

		bool isActive()
		{
			return m_is_active;
		}

		double angle()
		{
			return m_direction * amplitude_x * glm::sin(frequency * time_since_start());
		}

		void update()
		{
			if (time_since_start() >= duration_s)
			{
				if (m_should_stop)
				{
					m_is_active = false;
					m_should_stop = false;
				}
				else
				{
					m_start_time = std::chrono::steady_clock::now().time_since_epoch();
					m_direction = -m_direction;
				}
			}
		}

	private:

		bool m_is_active = false;
		bool m_should_stop = false;
		int m_direction = 1;
		std::chrono::nanoseconds m_start_time = std::chrono::nanoseconds(0);

		constexpr static inline double amplitude_x = 0.5;

		constexpr static inline double duration_s = 0.35;

		constexpr static inline double frequency = glm::pi<double>() / duration_s;

	};

	class AttackAnimation
	{

	public:

		AttackAnimation() = default;
		~AttackAnimation() = default;

		void start()
		{
			m_is_active = true;
			m_start_time = std::chrono::steady_clock::now().time_since_epoch();
			m_should_stop = true;
		}

		void stop()
		{
			m_should_stop = true;
		}

		double time_since_start()
		{
			return static_cast<double>((std::chrono::steady_clock::now().time_since_epoch() - m_start_time).count()) / 1e9;
		}

		bool isActive()
		{
			return m_is_active;
		}

		double angleX()
		{
			return amplitude_x * glm::sin(frequency * time_since_start());
		}

		double angleY()
		{
			return amplitude_y * glm::sin(frequency * time_since_start() * 2.0);
		}

		double angleZ()
		{
			return amplitude_z * glm::sin(frequency * time_since_start() * 2.0);
		}

		void update()
		{
			if (time_since_start() >= duration_s)
			{
				if (m_should_stop)
				{
					m_is_active = false;
					m_should_stop = false;
				}
				else
				{
					m_start_time = std::chrono::steady_clock::now().time_since_epoch();
				}
			}
		}

	private:

		bool m_is_active = false;
		bool m_should_stop = false;
		std::chrono::nanoseconds m_start_time = std::chrono::nanoseconds(0);

		constexpr static inline double amplitude_x = 1.0;
		constexpr static inline double amplitude_y = -0.8;
		constexpr static inline double amplitude_z = 0.3;

		constexpr static inline double duration_s = 0.2;

		constexpr static inline double frequency = glm::pi<double>() / duration_s;

	};

};
