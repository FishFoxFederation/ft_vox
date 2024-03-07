#pragma once

#include "VulkanAPI.hpp"

#include <string>
#include <unordered_map>
#include <mutex>
#include <any>
#include <atomic>

class DebugGui
{

public:

	DebugGui(VulkanAPI & vulkanAPI);

	~DebugGui();

	void updateImGui();

	static inline std::atomic<uint32_t> fps = 0;
	static inline std::atomic<uint64_t> triangle_count = 0;
	static inline std::atomic<float> cpu_time = 0.0f;

	static void pushFrameTime(float frame_time);

private:

	VulkanAPI & vk;

	static inline std::array<float, 100> frame_time_history;
	static inline std::mutex frame_time_history_mutex;

};
