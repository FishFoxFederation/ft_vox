#include "VulkanAPI.hpp"

#include <cstring>

uint64_t VulkanAPI::createMesh(
	const CreateMeshData & data
)
{
#define ADD_INDEX \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 3); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 1);

	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;
	const Chunk & chunk = *data.chunks[CreateMeshData::NEUT][CreateMeshData::NEUT][CreateMeshData::NEUT];
	const Chunk * x_pos_chunk = data.chunks[CreateMeshData::POS][CreateMeshData::NEUT][CreateMeshData::NEUT];
	const Chunk * x_neg_chunk = data.chunks[CreateMeshData::NEG][CreateMeshData::NEUT][CreateMeshData::NEUT];
	const Chunk * y_pos_chunk = data.chunks[CreateMeshData::NEUT][CreateMeshData::POS][CreateMeshData::NEUT];
	const Chunk * y_neg_chunk = data.chunks[CreateMeshData::NEUT][CreateMeshData::NEG][CreateMeshData::NEUT];
	const Chunk * z_pos_chunk = data.chunks[CreateMeshData::NEUT][CreateMeshData::NEUT][CreateMeshData::POS];
	const Chunk * z_neg_chunk = data.chunks[CreateMeshData::NEUT][CreateMeshData::NEUT][CreateMeshData::NEG];

	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				BlockID block_id = chunk.getBlock(x, y, z);

				if (block_id != BlockID::Air)
				{
					Data block_data = Block::getData(block_id);

					// check right neighbor (x + 1)
					if (
						(x < CHUNK_SIZE - 1 && chunk.getBlock(x + 1, y, z) == BlockID::Air)
						|| (x == CHUNK_SIZE - 1 && x_pos_chunk != nullptr && x_pos_chunk->getBlock(0, y, z) == BlockID::Air)
						// || (x == CHUNK_SIZE - 1 && x_pos_chunk == nullptr)
						// || (x == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x + 1, y, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture.right});
						vertices.push_back({{x + 1, y, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture.right});
						vertices.push_back({{x + 1, y + 1, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture.right});
						vertices.push_back({{x + 1, y + 1, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture.right});
						ADD_INDEX
					}

					// check left neighbor (x - 1)
					if (
						(x > 0 && chunk.getBlock(x - 1, y, z) == BlockID::Air)
						|| (x == 0 && x_neg_chunk != nullptr && x_neg_chunk->getBlock(CHUNK_SIZE - 1, y, z) == BlockID::Air)
						// || (x == 0 && x_neg_chunk == nullptr)
						// || (x == 0)
					)
					{
						vertices.push_back({{x, y, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture.left});
						vertices.push_back({{x, y, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture.left});
						vertices.push_back({{x, y + 1, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture.left});
						vertices.push_back({{x, y + 1, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture.left});
						ADD_INDEX
					}

					// check top neighbor (y + 1)
					if (
						(y < CHUNK_SIZE - 1 && chunk.getBlock(x, y + 1, z) == BlockID::Air)
						|| (y == CHUNK_SIZE - 1 && y_pos_chunk != nullptr && y_pos_chunk->getBlock(x, 0, z) == BlockID::Air)
						// || (y == CHUNK_SIZE - 1 && y_pos_chunk == nullptr)
						// || (y == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x + 1, y + 1, z}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture.top});
						vertices.push_back({{x, y + 1, z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture.top});
						vertices.push_back({{x, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture.top});
						vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture.top});
						ADD_INDEX
					}

					// check bottom neighbor (y - 1)
					if (
						(y > 0 && chunk.getBlock(x, y - 1, z) == BlockID::Air)
						|| (y == 0 && y_neg_chunk != nullptr && y_neg_chunk->getBlock(x, CHUNK_SIZE - 1, z) == BlockID::Air)
						// || (y == 0 && y_neg_chunk == nullptr)
						// || (y == 0)
					)
					{
						vertices.push_back({{x, y, z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture.bottom});
						vertices.push_back({{x + 1, y, z}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture.bottom});
						vertices.push_back({{x + 1, y, z + 1}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture.bottom});
						vertices.push_back({{x, y, z + 1}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture.bottom});
						ADD_INDEX
					}

					// check back neighbor (z + 1)
					if (
						(z < CHUNK_SIZE - 1 && chunk.getBlock(x, y, z + 1) == BlockID::Air)
						|| (z == CHUNK_SIZE - 1 && z_pos_chunk != nullptr && z_pos_chunk->getBlock(x, y, 0) == BlockID::Air)
						// || (z == CHUNK_SIZE - 1 && z_pos_chunk == nullptr)
						// || (z == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x, y, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, block_data.texture.front});
						vertices.push_back({{x + 1, y, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, block_data.texture.front});
						vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, block_data.texture.front});
						vertices.push_back({{x, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, block_data.texture.front});
						ADD_INDEX
					}

					// check front neighbor (z - 1)
					if (
						(z > 0 && chunk.getBlock(x, y, z - 1) == BlockID::Air)
						|| (z == 0 && z_neg_chunk != nullptr && z_neg_chunk->getBlock(x, y, CHUNK_SIZE - 1) == BlockID::Air)
						// || (z == 0 && z_neg_chunk == nullptr)
						// || (z == 0)
					)
					{
						vertices.push_back({{x + 1, y, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, block_data.texture.back});
						vertices.push_back({{x, y, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, block_data.texture.back});
						vertices.push_back({{x, y + 1, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, block_data.texture.back});
						vertices.push_back({{x + 1, y + 1, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, block_data.texture.back});
						ADD_INDEX
					}
				}
			}
		}
	}

#undef ADD_INDEX

	if (vertices.empty())
	{
		return no_mesh_id;
	}

	return storeMesh(vertices, indices);
}

uint64_t VulkanAPI::storeMesh(const std::vector<BlockVertex> & vertices, const std::vector<uint32_t> & indices)
{
	Mesh mesh;
	VkDeviceSize vertex_size = sizeof(vertices[0]) * vertices.size();
	VkDeviceSize index_size = sizeof(indices[0]) * indices.size();
	VkDeviceSize buffer_size = vertex_size + index_size;

	std::lock_guard<std::mutex> lock(global_mutex);

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_buffer_memory
	);

	void * data;
	VK_CHECK(
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data),
		"Failed to map memory for vertex/index staging buffer."
	);
	std::memcpy(data, vertices.data(), static_cast<size_t>(vertex_size));
	std::memcpy(static_cast<char *>(data) + vertex_size, indices.data(), static_cast<size_t>(index_size));
	vkUnmapMemory(device, staging_buffer_memory);

	createBuffer(
		buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh.buffer,
		mesh.buffer_memory
	);

	copyBuffer(staging_buffer, mesh.buffer, buffer_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vma.freeMemory(device, staging_buffer_memory, nullptr);

	mesh.vertex_count = static_cast<uint32_t>(vertices.size());
	mesh.index_offset = vertex_size;
	mesh.index_count = static_cast<uint32_t>(indices.size());
	mesh.memory_size = buffer_size;
	// Debug<uint64_t>::add("mesh_memory_size", buffer_size);

	meshes.emplace(next_mesh_id, mesh);
	return next_mesh_id++;
}

void VulkanAPI::destroyMesh(const uint64_t & mesh_id)
{
	std::lock_guard<std::mutex> lock(global_mutex);
	mesh_ids_to_destroy.push_back(mesh_id);
	destroyMeshes();
}

void VulkanAPI::destroyMeshes(const std::vector<uint64_t> & mesh_ids)
{
	std::lock_guard<std::mutex> lock(global_mutex);
	mesh_ids_to_destroy.insert(mesh_ids_to_destroy.end(), mesh_ids.begin(), mesh_ids.end());
	destroyMeshes();
}

void VulkanAPI::destroyMeshes()
{
	std::vector<uint64_t> meshes_still_in_use;
	meshes_still_in_use.reserve(mesh_ids_to_destroy.size());
	for (auto & id: mesh_ids_to_destroy)
	{
		auto mesh = meshes.find(id);
		if (mesh != meshes.end() && mesh->second.is_used == false)
		{
			// LOG_INFO("Destroying mesh: " << id);
			vkDestroyBuffer(device, mesh->second.buffer, nullptr);
			vma.freeMemory(device, mesh->second.buffer_memory, nullptr);
			meshes.erase(mesh);
		}
		else
		{
			meshes_still_in_use.push_back(id);
		}
	}
	mesh_ids_to_destroy = std::move(meshes_still_in_use);
}
