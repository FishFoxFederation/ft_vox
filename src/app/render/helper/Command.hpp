#pragma once

#include "vk_define.hpp"

#include <vector>

class SingleTimeCommand
{

public:

	SingleTimeCommand(
		VkDevice device,
		VkCommandPool command_pool,
		VkQueue queue
	);

	SingleTimeCommand(const SingleTimeCommand &) = delete;
	SingleTimeCommand & operator=(const SingleTimeCommand &) = delete;

	SingleTimeCommand(SingleTimeCommand && other) noexcept;
	SingleTimeCommand & operator=(SingleTimeCommand && other) noexcept;

	~SingleTimeCommand();

	void end();

	operator VkCommandBuffer() const
	{
		return m_command_buffer;
	}

private:

	VkDevice m_device;
	VkCommandPool m_command_pool;
	VkQueue m_queue;
	VkCommandBuffer m_command_buffer;

};