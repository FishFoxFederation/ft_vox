#include "RenderAPI.hpp"

void RenderAPI::_createDrawBuffer()
{
	m_max_draw_count = 10000;

	m_draw_chunk_block_shadow_pass_buffer.resize(m_max_frames_in_flight);
	m_draw_chunk_block_light_pass_buffer.resize(m_max_frames_in_flight);

	const Buffer::CreateInfo buffer_info = {
		.size = m_max_draw_count * sizeof(VkDrawIndexedIndirectCommand),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				| VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
							| VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	};
	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		m_draw_chunk_block_shadow_pass_buffer[i].resize(shadow_maps_count);
		for (uint32_t j = 0; j < shadow_maps_count; j++)
		{
			m_draw_chunk_block_shadow_pass_buffer[i][j] = Buffer(device, physical_device, buffer_info);
		}

		m_draw_chunk_block_light_pass_buffer[i] = Buffer(device, physical_device, buffer_info);
	}
}

void RenderAPI::_destroyDrawBuffer()
{
	for (int i = 0; i < m_max_frames_in_flight; i++)
	{
		for (uint32_t j = 0; j < shadow_maps_count; j++)
		{
			m_draw_chunk_block_shadow_pass_buffer[i][j].clear();
		}
		m_draw_chunk_block_light_pass_buffer[i].clear();
	}
}
