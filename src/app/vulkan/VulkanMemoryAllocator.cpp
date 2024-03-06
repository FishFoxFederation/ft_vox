#include "VulkanMemoryAllocator.hpp"

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
	m_allocated_memory_size += pAllocateInfo->allocationSize;
	m_allocated_memory[*pMemory] = pAllocateInfo->allocationSize;
	return vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

void VulaknMemoryAllocator::freeMemory(
	VkDevice device,
	VkDeviceMemory memory,
	const VkAllocationCallbacks * pAllocator
)
{
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