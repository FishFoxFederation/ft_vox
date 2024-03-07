#include "VulkanMemoryAllocator.hpp"
#include "logger.hpp"

VulaknMemoryAllocator::VulaknMemoryAllocator()
{
}

VulaknMemoryAllocator::~VulaknMemoryAllocator()
{
}

VkResult VulaknMemoryAllocator::allocateMemory(
	VkDevice device,
	const VkMemoryAllocateInfo * pAllocateInfo,
	const VkAllocationCallbacks * pAllocator,
	VkDeviceMemory * pMemory
)
{
	VkResult result = vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocate memory");
		return result;
	}
	m_allocated_memory_size += pAllocateInfo->allocationSize;
	m_allocated_memory[*pMemory] = pAllocateInfo->allocationSize;
	return result;
}

void VulaknMemoryAllocator::freeMemory(
	VkDevice device,
	VkDeviceMemory memory,
	const VkAllocationCallbacks * pAllocator
)
{
	if (m_allocated_memory.find(memory) == m_allocated_memory.end())
	{
		LOG_ERROR("Memory not found");
	}
	m_allocated_memory_size -= m_allocated_memory[memory];
	m_allocated_memory.erase(memory);
	vkFreeMemory(device, memory, pAllocator);
}

VkDeviceSize VulaknMemoryAllocator::allocatedMemorySize() const
{
	return m_allocated_memory_size;
}

uint32_t VulaknMemoryAllocator::allocatedMemoryCount() const
{
	return m_allocated_memory.size();
}