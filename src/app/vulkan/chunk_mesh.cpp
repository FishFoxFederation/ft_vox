#include "VulkanAPI.hpp"

#include <cstring>

uint64_t VulkanAPI::createMesh(
	const Chunk & chunk,
	const Chunk * x_pos_chunk,
	const Chunk * x_neg_chunk,
	const Chunk * y_pos_chunk,
	const Chunk * y_neg_chunk,
	const Chunk * z_pos_chunk,
	const Chunk * z_neg_chunk
)
{
	(void)y_pos_chunk;
	(void)x_pos_chunk;
	(void)z_pos_chunk;
	(void)x_neg_chunk;
	(void)y_neg_chunk;
	(void)z_neg_chunk;
	std::lock_guard<std::mutex> lock(global_mutex);
#define ADD_INDEX \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 3); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 1);

	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;

	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				Block block = chunk.getBlock(x, y, z);

				if (block != Block::Air)
				{

					uint32_t texture_index = 0;
					switch (block)
					{
					case Block::Grass: texture_index = 0; break;
					case Block::Stone: texture_index = 1; break;
					default:
						break;
					}

					// check right neighbor (x + 1)
					if (
						(x < CHUNK_SIZE - 1 && chunk.getBlock(x + 1, y, z) == Block::Air)
						|| (x == CHUNK_SIZE - 1 && x_pos_chunk != nullptr && x_pos_chunk->getBlock(0, y, z) == Block::Air)
						// || (x == CHUNK_SIZE - 1 && x_pos_chunk == nullptr)
						// || (x == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x + 1, y, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, texture_index});
						vertices.push_back({{x + 1, y, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, texture_index});
						vertices.push_back({{x + 1, y + 1, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, texture_index});
						vertices.push_back({{x + 1, y + 1, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, texture_index});
						ADD_INDEX
					}

					// check left neighbor (x - 1)
					if (
						(x > 0 && chunk.getBlock(x - 1, y, z) == Block::Air)
						|| (x == 0 && x_neg_chunk != nullptr && x_neg_chunk->getBlock(CHUNK_SIZE - 1, y, z) == Block::Air)
						// || (x == 0 && x_neg_chunk == nullptr)
						// || (x == 0)
					)
					{
						vertices.push_back({{x, y, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, texture_index});
						vertices.push_back({{x, y, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, texture_index});
						vertices.push_back({{x, y + 1, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, texture_index});
						vertices.push_back({{x, y + 1, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, texture_index});
						ADD_INDEX
					}

					// check top neighbor (y + 1)
					if (
						(y < CHUNK_SIZE - 1 && chunk.getBlock(x, y + 1, z) == Block::Air)
						|| (y == CHUNK_SIZE - 1 && y_pos_chunk != nullptr && y_pos_chunk->getBlock(x, 0, z) == Block::Air)
						// || (y == CHUNK_SIZE - 1 && y_pos_chunk == nullptr)
						// || (y == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x + 1, y + 1, z}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, texture_index});
						vertices.push_back({{x, y + 1, z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, texture_index});
						vertices.push_back({{x, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, texture_index});
						vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, texture_index});
						ADD_INDEX
					}

					// check bottom neighbor (y - 1)
					if (
						(y > 0 && chunk.getBlock(x, y - 1, z) == Block::Air)
						|| (y == 0 && y_neg_chunk != nullptr && y_neg_chunk->getBlock(x, CHUNK_SIZE - 1, z) == Block::Air)
						// || (y == 0 && y_neg_chunk == nullptr)
						// || (y == 0)
					)
					{
						vertices.push_back({{x, y, z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, texture_index});
						vertices.push_back({{x + 1, y, z}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, texture_index});
						vertices.push_back({{x + 1, y, z + 1}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, texture_index});
						vertices.push_back({{x, y, z + 1}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, texture_index});
						ADD_INDEX
					}

					// check back neighbor (z + 1)
					if (
						(z < CHUNK_SIZE - 1 && chunk.getBlock(x, y, z + 1) == Block::Air)
						|| (z == CHUNK_SIZE - 1 && z_pos_chunk != nullptr && z_pos_chunk->getBlock(x, y, 0) == Block::Air)
						// || (z == CHUNK_SIZE - 1 && z_pos_chunk == nullptr)
						// || (z == CHUNK_SIZE - 1)
					)
					{
						vertices.push_back({{x, y, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, texture_index});
						vertices.push_back({{x + 1, y, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, texture_index});
						vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, texture_index});
						vertices.push_back({{x, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, texture_index});
						ADD_INDEX
					}

					// check front neighbor (z - 1)
					if (
						(z > 0 && chunk.getBlock(x, y, z - 1) == Block::Air)
						|| (z == 0 && z_neg_chunk != nullptr && z_neg_chunk->getBlock(x, y, CHUNK_SIZE - 1) == Block::Air)
						// || (z == 0 && z_neg_chunk == nullptr)
						// || (z == 0)
					)
					{
						vertices.push_back({{x + 1, y, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, texture_index});
						vertices.push_back({{x, y, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, texture_index});
						vertices.push_back({{x, y + 1, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, texture_index});
						vertices.push_back({{x + 1, y + 1, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, texture_index});
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