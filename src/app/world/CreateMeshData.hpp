#pragma once

#include "define.hpp"
#include "vk_define.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Chunk.hpp"
#include "hashes.hpp"

#include <unordered_map>

struct BlockVertex
{
	glm::ivec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	uint32_t texLayer;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(BlockVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SINT;
		attributeDescriptions[0].offset = offsetof(BlockVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(BlockVertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(BlockVertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = offsetof(BlockVertex, texLayer);

		return attributeDescriptions;
	}

	bool operator==(const BlockVertex& other) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

namespace std
{
	template<> struct hash<BlockVertex>
	{
		size_t operator()(const BlockVertex & vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

typedef std::unordered_map<glm::ivec3, Chunk> ChunkMap;

struct CreateMeshData
{
	CreateMeshData(const glm::ivec3 & pos, ChunkMap & chunk_map)
	{
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					glm::ivec3 chunk_pos = pos + glm::ivec3(x, y, z);
					const auto & it = chunk_map.find(chunk_pos);

					if (it != chunk_map.end())
						chunks[x + 1][y + 1][z + 1] = &it->second;
					else
						chunks[x + 1][y + 1][z + 1] = nullptr;
				}
			}
		}
	}

	std::array<std::array<std::array<Chunk *, 3> , 3>, 3> chunks;

	enum
	{
		POS = 2,
		NEUT = 1,
		NEG = 0
	};

	void create()
	{
		#define ADD_INDEX \
		indices.push_back(vertices.size() - 4); \
		indices.push_back(vertices.size() - 3); \
		indices.push_back(vertices.size() - 2); \
		indices.push_back(vertices.size() - 4); \
		indices.push_back(vertices.size() - 2); \
		indices.push_back(vertices.size() - 1);

		const Chunk & chunk = *chunks[NEUT][NEUT][NEUT];
		const Chunk * x_pos_chunk = chunks[POS][NEUT][NEUT];
		const Chunk * x_neg_chunk = chunks[NEG][NEUT][NEUT];
		const Chunk * y_pos_chunk = chunks[NEUT][POS][NEUT];
		const Chunk * y_neg_chunk = chunks[NEUT][NEG][NEUT];
		const Chunk * z_pos_chunk = chunks[NEUT][NEUT][POS];
		const Chunk * z_neg_chunk = chunks[NEUT][NEUT][NEG];

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
	}

	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;
};
