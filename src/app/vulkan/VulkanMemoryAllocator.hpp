#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <map>


class VulaknMemoryAllocator
{

public:

	VulaknMemoryAllocator();
	~VulaknMemoryAllocator();

	VulaknMemoryAllocator(const VulaknMemoryAllocator &) = delete;
	VulaknMemoryAllocator(VulaknMemoryAllocator &&) = delete;
	VulaknMemoryAllocator & operator=(const VulaknMemoryAllocator &) = delete;
	VulaknMemoryAllocator & operator=(VulaknMemoryAllocator &&) = delete;

	VkResult allocateMemory(
		VkDevice device,
		const VkMemoryAllocateInfo * pAllocateInfo,
		const VkAllocationCallbacks * pAllocator,
		VkDeviceMemory * pMemory
	);

	void freeMemory(
		VkDevice device,
		VkDeviceMemory memory,
		const VkAllocationCallbacks * pAllocator
	);

	VkDeviceSize allocatedMemorySize() const;

	uint32_t allocatedMemoryCount() const;

private:

	std::map<VkDeviceMemory, VkDeviceSize> m_allocated_memory;

	VkDeviceSize m_allocated_memory_size = 0;

};