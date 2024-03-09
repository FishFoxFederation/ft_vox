#pragma once

#include "VulkanAPI.hpp"

#include <string>
#include <unordered_map>
#include <mutex>
#include <any>
#include <atomic>

template <typename T>
class Atomic
{
public:
	Atomic() = default;
	Atomic(T value) : m_value(value) {}

	T get() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_value;
	}

	void set(T value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_value = value;
	}

private:

	T m_value;
	mutable std::mutex m_mutex;

};

class DebugGui
{

public:

	DebugGui(VulkanAPI & vulkanAPI);

	~DebugGui();

	void updateImGui();

	static inline std::atomic<uint32_t> fps = 0;
	static inline std::atomic<uint64_t> triangle_count = 0;
	static inline std::atomic<float> cpu_time = 0.0f;

	static inline Atomic<glm::vec3> camera_last_position;
	static inline Atomic<glm::vec3> camera_new_position;
	static inline Atomic<glm::vec3> camera_displacement;
	static inline Atomic<glm::vec3> camera_position_sub_last_position;
	static inline std::atomic<float> camera_update_time = 0.0f;

	static void pushFrameTime(float frame_time);

private:

	VulkanAPI & vk;

	static inline std::array<float, 100> frame_time_history;
	static inline std::mutex frame_time_history_mutex;

};
