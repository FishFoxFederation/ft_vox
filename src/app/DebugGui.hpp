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
	static inline std::atomic<uint64_t> mesh_memory_size = 0;

private:

	VulkanAPI & vk;

};
