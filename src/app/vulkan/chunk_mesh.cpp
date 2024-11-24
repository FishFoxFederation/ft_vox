#include "VulkanAPI.hpp"
#include "DebugGui.hpp"

#include "Tracy.hpp"

VulkanAPI::InstanceId VulkanAPI::addChunkToScene(
	const ChunkMeshCreateInfo & mesh_info
)
{
	if (mesh_info.block_index.empty() && mesh_info.water_index.empty())
	{
		return m_null_instance_id;
	}

	// std::lock_guard global_lock(global_mutex);
	std::lock_guard lock(m_render_data_update_mutex);

	if (m_free_chunk_ids.empty())
	{
		throw std::runtime_error("addChunkToScene: there is no space left to add a chunk in the scene. "
								"Go ask sgaubert to implement the dynamic resize of instance_data_buffers. "
								"Increase instance_data_max_count in the code waiting for him to do his job.");
	}

	const InstanceId instance_id = m_free_chunk_ids.front();
	m_free_chunk_ids.pop_front();

	m_chunk_to_create[instance_id] = std::move(mesh_info);

	return instance_id;
}

void VulkanAPI::removeChunkFromScene(const uint64_t chunk_id)
{
	std::lock_guard lock(m_render_data_update_mutex);

	if (!m_chunks_in_scene.contains(chunk_id))
	{
		return;
	}

	m_chunks_in_scene_rendered.erase(chunk_id);
	m_chunk_instance_to_destroy.push_back(chunk_id);

	_deleteUnusedChunks();
}

void VulkanAPI::_setupChunksRessources()
{
	for (uint32_t i = 1; i < instance_data_max_count; i++)
	{
		m_free_chunk_ids.push_back(i);
	}

	_resizeChunksIndicesBuffer(100000 * sizeof(uint32_t));
}

void VulkanAPI::_resizeChunksIndicesBuffer(const VkDeviceSize & size)
{
	// need to wait because m_chunks_indices_buffer is used by in flight frame
	vkDeviceWaitIdle(device);
	
	Buffer::CreateInfo buffer_info = {
		.size = m_chunks_indices_buffer.size() + size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
				| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	Buffer new_buffer = Buffer(device, physical_device, buffer_info);

	if (m_chunks_indices_buffer.size() != 0)
	{
		const VkBufferCopy buffer_copy = {
			.size = m_chunks_indices_buffer.size()
		};
		_copyBuffer(
			m_chunks_indices_buffer.buffer,
			new_buffer.buffer,
			buffer_copy
		);
	}

	m_chunks_indices_buffer = std::move(new_buffer);
	m_chunks_indices_buffer_memory_range.add(size);
}

void VulkanAPI::_deleteUnusedChunks()
{
	ZoneScoped;

	std::vector<InstanceId> chunks_still_in_use;
	chunks_still_in_use.reserve(m_chunk_instance_to_destroy.size());
	for (auto & id: m_chunk_instance_to_destroy)
	{
		ChunkMeshesInfo & chunk = m_chunks_in_scene[id];
		if (chunk.is_used == false)
		{
			m_chunks_in_scene.erase(id);
			m_free_chunk_ids.push_front(id);
		}
		else
		{
			chunks_still_in_use.push_back(id);
		}
	}
	m_chunk_instance_to_destroy = std::move(chunks_still_in_use);
}
