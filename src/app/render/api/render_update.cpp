#include "RenderAPI.hpp"

void RenderAPI::setCamera(const Camera & camera)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_camera_update = camera;
}

void RenderAPI::toggleDebugText()
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_show_debug_text_update = !m_show_debug_text_update;
}

void RenderAPI::setTargetBlock(const std::optional<glm::vec3> & target_block)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_target_block_update = target_block;
}


void RenderAPI::addPlayer(const uint64_t id, const PlayerRenderData & player_data)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id, player_data]()
	{
		m_players[id] = player_data;
	});
}

void RenderAPI::removePlayer(const uint64_t id)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id]()
	{
		m_players.erase(id);
	});
}

void RenderAPI::updatePlayer(const uint64_t id, std::function<void(PlayerRenderData &)> fct)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id, fct]()
	{
		fct(m_players[id]);
	});
}


void RenderAPI::addEntity(const uint64_t id, const MeshRenderData & player_data)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id, player_data]()
	{
		m_entities[id] = player_data;
	});
}

void RenderAPI::removeEntity(const uint64_t id)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id]()
	{
		m_entities.erase(id);
	});
}

void RenderAPI::updateEntity(const uint64_t id, std::function<void(MeshRenderData &)> fct)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, id, fct]()
	{
		fct(m_entities[id]);
	});
}


void RenderAPI::setToolbarItem(const int index, const ItemInfo::Type type)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, index, type]()
	{
		m_toolbar_items[index] = type;
	});
}

void RenderAPI::setToolbarCursor(const int index)
{
	std::lock_guard lock(m_render_data_update_mutex);
	m_update_functions.push_back([this, index]()
	{
		m_toolbar_cursor_index = index;
	});
}


void RenderAPI::_updateRenderData()
{
	std::lock_guard lock(m_render_data_update_mutex);

	glfwGetFramebufferSize(window, &m_window_width, &m_window_height);
	m_aspect_ratio = static_cast<double>(m_window_width) / static_cast<double>(m_window_height);
	m_camera_render_info = m_camera_update.getRenderInfo(m_aspect_ratio);

	m_camera_matrices.view = m_camera_render_info.view;
	m_camera_matrices.proj = m_clip * m_camera_render_info.projection;

	m_camera_matrices_fc.view = m_camera_render_info.view;
	m_camera_matrices_fc.proj = m_camera_render_info.projection;

	m_show_debug_text = m_show_debug_text_update;

	m_target_block = m_target_block_update;

	_updateChunksData();

	for (auto & fct: m_update_functions)
	{
		fct();
	}
	m_update_functions.clear();
}

void RenderAPI::_updateChunksData()
{
	for (auto & [instance_id, _]: m_chunk_to_delete)
	{
		m_chunks_in_scene_rendered.erase(instance_id);
		m_chunk_instance_to_destroy.push_back(instance_id);

		_deleteUnusedChunks();
	}
	m_chunk_to_delete.clear();

	for (auto & [instance_id, mesh_info]: m_chunk_to_create)
	{
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

			.model = mesh_info.model
		};

		m_chunks_in_scene[instance_id] = std::move(chunk_meshes_info);
		m_chunks_in_scene_rendered[instance_id] = mesh_info.model;
	}

	m_chunk_to_create.clear();
}

void RenderAPI::_updatePlayersData()
{
	for (auto & [id, data] : m_players)
	{
		data.walk_animation.update();
		data.attack_animation.update();
	}
}
