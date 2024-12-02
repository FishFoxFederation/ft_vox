#pragma once

#include <vulkan/vulkan.h>

#include "Tracy.hpp"

#include <map>
#include <mutex>

/**
 * @brief Singleton class to keep track of allocated memory
 *
 */
class VulkanMemoryAllocator
{

public:

	static VulkanMemoryAllocator & getInstance()
	{
		static VulkanMemoryAllocator instance;
		return instance;
	}

	VulkanMemoryAllocator(const VulkanMemoryAllocator &) = delete;
	VulkanMemoryAllocator(VulkanMemoryAllocator &&) = delete;
	VulkanMemoryAllocator & operator=(const VulkanMemoryAllocator &) = delete;
	VulkanMemoryAllocator & operator=(VulkanMemoryAllocator &&) = delete;

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

	VulkanMemoryAllocator();
	~VulkanMemoryAllocator();

	std::map<VkDeviceMemory, VkDeviceSize> m_allocated_memory;

	VkDeviceSize m_allocated_memory_size = 0;

	mutable TracyLockableN(std::mutex, m_mutex, "Vulkan Memory Allocator");
};
