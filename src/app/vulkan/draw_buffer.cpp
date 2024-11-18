#include "VulkanAPI.hpp"

void VulkanAPI::_createDrawBuffer()
{
	m_max_draw_count = 10000;

	m_draw_chunk_block_light_pass_buffer.resize(max_frames_in_flight);

	const Buffer::CreateInfo buffer_info = {
		.size = m_max_draw_count * sizeof(VkDrawIndexedIndirectCommand),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				| VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
							| VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	};
	for (int i = 0; i < max_frames_in_flight; i++)
	{
		m_draw_chunk_block_light_pass_buffer[i] = Buffer(device, physical_device, buffer_info);
	}
}

void VulkanAPI::_destroyDrawBuffer()
{
	for (int i = 0; i < max_frames_in_flight; i++)
	{
		m_draw_chunk_block_light_pass_buffer[i].clear();
	}
}
