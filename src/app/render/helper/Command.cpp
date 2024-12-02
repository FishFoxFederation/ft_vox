#include "Command.hpp"
#include "logger.hpp"

#include <cassert>

SingleTimeCommand::SingleTimeCommand(
	VkDevice device,
	VkCommandPool command_pool,
	VkQueue queue
):
	m_device(device),
	m_command_pool(command_pool),
	m_queue(queue)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = m_command_pool;
	alloc_info.commandBufferCount = 1;

	VK_CHECK(
		vkAllocateCommandBuffers(m_device, &alloc_info, &m_command_buffer),
		"Falied to allocate single time command buffer"
	);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(
		vkBeginCommandBuffer(m_command_buffer, &begin_info),
		"Failed to begin single time command buffer"
	);
}

SingleTimeCommand::SingleTimeCommand(SingleTimeCommand && other) noexcept:
	m_device(other.m_device),
	m_command_pool(other.m_command_pool),
	m_queue(other.m_queue),
	m_command_buffer(other.m_command_buffer)
{
	other.m_device = VK_NULL_HANDLE;
	other.m_command_pool = VK_NULL_HANDLE;
	other.m_queue = VK_NULL_HANDLE;
	other.m_command_buffer = VK_NULL_HANDLE;
}

SingleTimeCommand & SingleTimeCommand::operator=(SingleTimeCommand && other) noexcept
{
	if (this != &other)
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(m_device, m_command_pool, 1, &m_command_buffer);
		}

		m_device = other.m_device;
		m_command_pool = other.m_command_pool;
		m_queue = other.m_queue;
		m_command_buffer = other.m_command_buffer;

		other.m_device = VK_NULL_HANDLE;
		other.m_command_pool = VK_NULL_HANDLE;
		other.m_queue = VK_NULL_HANDLE;
		other.m_command_buffer = VK_NULL_HANDLE;
	}

	return *this;
}

SingleTimeCommand::~SingleTimeCommand()
{
	if (m_command_buffer != VK_NULL_HANDLE)
	{
		end();
	}
}

void SingleTimeCommand::end()
{
	assert(m_command_buffer != VK_NULL_HANDLE && "Cannot end null command buffer");

	VK_CHECK(
		vkEndCommandBuffer(m_command_buffer),
		"Failed to end single time command buffer"
	);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffer;

	VK_CHECK(
		vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE),
		"Failed to submit single time command buffer"
	);

	VK_CHECK(
		vkQueueWaitIdle(m_queue),
		"Failed to wait for queue to idle"
	);

	vkFreeCommandBuffers(m_device, m_command_pool, 1, &m_command_buffer);
	m_command_buffer = VK_NULL_HANDLE;
}
