#include "VulkanMemoryAllocator.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

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
	std::unique_lock<std::mutex> lock(m_mutex);
	VkResult result = vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocate memory");
		return result;
	}
	m_allocated_memory_size += pAllocateInfo->allocationSize;
	m_allocated_memory[*pMemory] = pAllocateInfo->allocationSize;
	DebugGui::gpu_allocated_memory = m_allocated_memory_size;
	return result;
}

void VulaknMemoryAllocator::freeMemory(
	VkDevice device,
	VkDeviceMemory memory,
	const VkAllocationCallbacks * pAllocator
)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_allocated_memory.find(memory) == m_allocated_memory.end())
	{
		LOG_ERROR("Trying to free memory that was not allocated by this allocator");
	}
	m_allocated_memory_size -= m_allocated_memory[memory];
	m_allocated_memory.erase(memory);
	vkFreeMemory(device, memory, pAllocator);
	DebugGui::gpu_allocated_memory = m_allocated_memory_size;
}

VkDeviceSize VulaknMemoryAllocator::allocatedMemorySize() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_allocated_memory_size;
}

uint32_t VulaknMemoryAllocator::allocatedMemoryCount() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_allocated_memory.size();
}
