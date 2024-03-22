#pragma once

#include "vk_define.hpp"

namespace vk_helper
{
	uint32_t findMemoryType(
		VkPhysicalDevice physical_device,
		uint32_t type_filter,
		VkMemoryPropertyFlags properties
	);
}