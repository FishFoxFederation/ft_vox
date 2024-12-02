#pragma once

#include "vk_define.hpp"

namespace vk_helper
{
	uint32_t findMemoryType(
		VkPhysicalDevice physical_device,
		uint32_t type_filter,
		VkMemoryPropertyFlags properties
	);

	void setImageLayout(
		VkCommandBuffer command_buffer,
		VkImage image,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		VkImageSubresourceRange subresource_range,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask
	);

}