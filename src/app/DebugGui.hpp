#pragma once

#include "VulkanAPI.hpp"
#include "Camera.hpp"

#include <string>
#include <mutex>
#include <atomic>
#include <array>
#include <algorithm>
#include <numeric>

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

template <typename T, int N>
class History
{

public:

	void push(T value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_sum += value;
		m_sum -= m_history[0];
		std::shift_left(m_history.begin(), m_history.end(), 1);
		m_history[N - 1] = value;
	}

	T average() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_sum / N;
	}

	std::unique_lock<std::mutex> lock() const
	{
		return std::unique_lock<std::mutex>(m_mutex);
	}

	T * data()
	{
		return m_history.data();
	}

	int size() const
	{
		return N;
	}

private:

	std::array<T, N> m_history;
	T m_sum = 0;
	mutable std::mutex m_mutex;

};

class DebugGui
{

public:

	DebugGui(VulkanAPI & vulkanAPI);

	~DebugGui();

	void updateImGui();

	static inline std::atomic<uint32_t> fps = 0;
	static inline std::atomic<uint64_t> rendered_triangles = 0;

	static inline Atomic<glm::vec3> camera_last_position;
	static inline Atomic<glm::vec3> camera_new_position;
	static inline Atomic<glm::vec3> camera_displacement;
	static inline Atomic<glm::vec3> camera_position_sub_last_position;
	static inline std::atomic<float> camera_update_time = 0.0f;

	// Render Thread times
	static inline History<float, 100> frame_time_history;
	static inline History<float, 100> cpu_time_history;

	// Camera
	static inline Atomic<glm::vec3> camera_position = glm::vec3(0.0f, 0.0f, 0.0f);
	static inline std::atomic<float> pitch = 1.0f;
	static inline std::atomic<float> yaw = 1.0f;
	static inline std::atomic<float> near_plane = 1.0f;
	static inline std::atomic<float> far_plane = 30.0f;
	static inline std::atomic<float> fov = 80.0f;

private:

	VulkanAPI & vk;


};
