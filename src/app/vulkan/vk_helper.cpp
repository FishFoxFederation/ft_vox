#include "vk_helper.hpp"

namespace vk_helper
{
	uint32_t findMemoryType(
		VkPhysicalDevice physical_device,
		uint32_t type_filter,
		VkMemoryPropertyFlags properties
	)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if (
				(type_filter & (1 << i)) &&
				(mem_properties.memoryTypes[i].propertyFlags & properties) == properties
			)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type.");
	}
}