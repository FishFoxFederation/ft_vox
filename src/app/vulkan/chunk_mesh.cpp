#include "VulkanAPI.hpp"
#include "DebugGui.hpp"

#include "Tracy.hpp"

VulkanAPI::InstanceId VulkanAPI::addChunkToScene(
	const ChunkMeshCreateInfo & mesh_info,
	const glm::dmat4 & model
)
{
	if (mesh_info.block_index.empty() && mesh_info.water_index.empty())
	{
		return m_null_instance_id;
	}

	std::lock_guard global_lock(global_mutex);

	if (m_free_chunk_ids.empty())
	{
		throw std::runtime_error("addChunkToScene: there is no space left to add a chunk in the scene. "
								"Go ask sgaubert to implement the dynamic resize of instance_data_buffers. "
								"Increase instance_data_max_count in the code waiting for him to do his job.");
	}

	const VkDeviceSize block_index_size = mesh_info.block_index.size() * sizeof(uint32_t);
	const VkDeviceSize water_index_size = mesh_info.water_index.size() * sizeof(uint32_t);
	const VkDeviceSize index_size = block_index_size + water_index_size;

	// get indices offset
	VkDeviceSize index_offset = m_chunks_indices_buffer_memory_range.alloc(index_size);
	if (index_offset == m_chunks_indices_buffer_memory_range.capacity())
	{
		const VkDeviceSize added_size = std::max(index_size, 100000 * sizeof(uint32_t));
		_resizeChunksIndicesBuffer(added_size);
		index_offset = m_chunks_indices_buffer_memory_range.alloc(index_size);
	}

	// create index staging buffer
	const Buffer::CreateInfo staging_index_buffer_info = {
		.size = index_size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer staging_index_buffer = Buffer(device, physical_device, staging_index_buffer_info);

	// copy index data to the staging buffer
	std::memcpy(
		staging_index_buffer.mappedMemory(),
		mesh_info.block_index.data(),
		static_cast<size_t>(block_index_size)
	);
	std::memcpy(
		static_cast<uint8_t *>(staging_index_buffer.mappedMemory()) + block_index_size,
		mesh_info.water_index.data(),
		static_cast<size_t>(water_index_size)
	);

	// copy staging index buffer to the index buffer
	const VkBufferCopy index_buffer_copy = {
		.srcOffset = 0,
		.dstOffset = index_offset,
		.size = index_size
	};
	_copyBuffer(
		staging_index_buffer.buffer,
		m_chunks_indices_buffer.buffer,
		index_buffer_copy
	);


	const VkDeviceSize block_vertex_size = mesh_info.block_vertex.size() * sizeof(BlockVertex);
	const VkDeviceSize water_vertex_size = mesh_info.water_vertex.size() * sizeof(BlockVertex);
	const VkDeviceSize vertex_size = block_vertex_size + water_vertex_size;

	// create vertex staging buffer
	const Buffer::CreateInfo staging_vertex_buffer_info = {
		.size = vertex_size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	Buffer staging_vertex_buffer = Buffer(device, physical_device, staging_vertex_buffer_info);

	// copy vertex data to the staging buffer
	std::memcpy(
		staging_vertex_buffer.mappedMemory(),
		mesh_info.block_vertex.data(),
		static_cast<size_t>(block_vertex_size)
	);
	std::memcpy(
		static_cast<uint8_t *>(staging_vertex_buffer.mappedMemory()) + block_vertex_size,
		mesh_info.water_vertex.data(),
		static_cast<size_t>(water_vertex_size)
	);

	// create vertex buffer
	const Buffer::CreateInfo vertex_buffer_info = {
		.size = vertex_size,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	Buffer vertex_buffer = Buffer(device, physical_device, vertex_buffer_info);

	VkBufferDeviceAddressInfoKHR address_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.pNext = nullptr,
		.buffer = vertex_buffer.buffer
	};
	const VkDeviceAddress vertex_buffer_address = vkGetBufferDeviceAddress(device, &address_info);

	// copy staging vertex buffer to the vertex buffer
	_copyBuffer(
		staging_vertex_buffer.buffer,
		vertex_buffer.buffer,
		{ 0, 0, vertex_size }
	);

	// finish creating chunk meshes info
	ChunkMeshesInfo chunk_meshes_info = {
		.vertex_buffer = std::move(vertex_buffer),

		.block_vertex_address = vertex_buffer_address,
		.block_index_offset = static_cast<uint32_t>(index_offset / sizeof(uint32_t)),
		.block_index_count = static_cast<uint32_t>(mesh_info.block_index.size()),

		.water_vertex_address = vertex_buffer_address + block_vertex_size,
		.water_index_offset = static_cast<uint32_t>((index_offset + block_index_size) / sizeof(uint32_t)),
		.water_index_count = static_cast<uint32_t>(mesh_info.water_index.size()),

		.model = model
	};

	const InstanceId instance_id = m_free_chunk_ids.front();
	m_free_chunk_ids.pop_front();

	{
		// std::lock_guard chunks_in_scene_lock(m_chunks_in_scene_mutex);
		m_chunks_in_scene[instance_id] = std::move(chunk_meshes_info);
		m_chunks_in_scene_rendered[instance_id] = model;
	}

	return instance_id;
}

void VulkanAPI::removeChunkFromScene(const uint64_t chunk_id)
{
	std::lock_guard global_lock(global_mutex);
	// std::lock_guard chunks_in_scene_lock(m_chunks_in_scene_mutex);

	if (!m_chunks_in_scene.contains(chunk_id))
	{
		return;
	}

	m_chunks_in_scene_rendered.erase(chunk_id);
	m_chunk_instance_to_destroy.push_back(chunk_id);

	_deleteUnusedChunks();
}

std::map<VulkanAPI::InstanceId, glm::dmat4> VulkanAPI::_getChunksInScene() const
{
	std::lock_guard global_lock(global_mutex);
	// std::lock_guard lock(m_chunks_in_scene_mutex);

	return m_chunks_in_scene_rendered;
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
