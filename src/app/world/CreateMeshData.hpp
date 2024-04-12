#pragma once

#include "define.hpp"
#include "vk_define.hpp"
#include "DebugGui.hpp"
#include "Timer.hpp"

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
	/**
	 * @brief Create a Mesh Data object
	 * 
	 * @param pos the position of the chunk in the chunk map
	 * @param size the number of chunks to create the mesh data
	 * @param chunk_map the chunk map
	 */
	CreateMeshData(const glm::ivec3 & pos, const glm::ivec3 & size, ChunkMap & chunk_map):
		chunks(size.x + 2, std::vector<std::vector<Chunk *>>(size.y + 2, std::vector<Chunk *>(size.z + 2, nullptr))),
		size(size)
	{
		for (int x = -1; x <= size.x; x++)
		{
			for (int y = -1; y <= size.y; y++)
			{
				for (int z = -1; z <= size.z; z++)
				{
					glm::ivec3 chunk_pos = pos + glm::ivec3(x, y, z);
					const auto & it = chunk_map.find(chunk_pos);

					if (it != chunk_map.end())
						chunks[x + 1][y + 1][z + 1] = &it->second;
				}
			}
		}
	}

	std::vector<std::vector<std::vector<Chunk *>>> chunks;
	glm::ivec3 size;

	enum
	{
		POS = 2,
		NEUT = 1,
		NEG = 0
	};

	/**
	 * @brief Get the block at a position relative to the starting chunk
	 *
	 */
	BlockID block(const int x, const int y, const int z)
	{
		const int chunk_x = (x + CHUNK_SIZE) / CHUNK_SIZE;
		const int chunk_y = (y + CHUNK_SIZE) / CHUNK_SIZE;
		const int chunk_z = (z + CHUNK_SIZE) / CHUNK_SIZE;

		const int block_x = (x + CHUNK_SIZE) % CHUNK_SIZE;
		const int block_y = (y + CHUNK_SIZE) % CHUNK_SIZE;
		const int block_z = (z + CHUNK_SIZE) % CHUNK_SIZE;

		if (chunks[chunk_x][chunk_y][chunk_z] == nullptr)
		{
			return BlockID::Air;
		}

		return chunks[chunk_x][chunk_y][chunk_z]->getBlock(block_x, block_y, block_z);
	}

	#define ADD_INDEX \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 3); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 4); \
	indices.push_back(vertices.size() - 2); \
	indices.push_back(vertices.size() - 1);

	void create_old()
	{
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int z = 0; z < CHUNK_SIZE; z++)
				{
					BlockID block_id = block(x, y, z);

					if (block_id != BlockID::Air)
					{
						Data block_data = Block::getData(block_id);

						// check right neighbor (x + 1)
						if (block(x + 1, y, z) == BlockID::Air)
						{
							vertices.push_back({{x + 1, y, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_RIGHT]});
							vertices.push_back({{x + 1, y, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_RIGHT]});
							vertices.push_back({{x + 1, y + 1, z}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_RIGHT]});
							vertices.push_back({{x + 1, y + 1, z + 1}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_RIGHT]});
							ADD_INDEX
						}

						// check left neighbor (x - 1)
						if (block(x - 1, y, z) == BlockID::Air)
						{
							vertices.push_back({{x, y, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_LEFT]});
							vertices.push_back({{x, y, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_LEFT]});
							vertices.push_back({{x, y + 1, z + 1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_LEFT]});
							vertices.push_back({{x, y + 1, z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_LEFT]});
							ADD_INDEX
						}

						// check top neighbor (y + 1)
						if (block(x, y + 1, z) == BlockID::Air)
						{
							vertices.push_back({{x + 1, y + 1, z}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_TOP]});
							vertices.push_back({{x, y + 1, z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_TOP]});
							vertices.push_back({{x, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_TOP]});
							vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_TOP]});
							ADD_INDEX
						}

						// check bottom neighbor (y - 1)
						if (block(x, y - 1, z) == BlockID::Air)
						{
							vertices.push_back({{x, y, z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_BOTTOM]});
							vertices.push_back({{x + 1, y, z}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_BOTTOM]});
							vertices.push_back({{x + 1, y, z + 1}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_BOTTOM]});
							vertices.push_back({{x, y, z + 1}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_BOTTOM]});
							ADD_INDEX
						}

						// check back neighbor (z + 1)
						if (block(x, y, z + 1) == BlockID::Air)
						{
							vertices.push_back({{x, y, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_FRONT]});
							vertices.push_back({{x + 1, y, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_FRONT]});
							vertices.push_back({{x + 1, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_FRONT]});
							vertices.push_back({{x, y + 1, z + 1}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_FRONT]});
							ADD_INDEX
						}

						// check front neighbor (z - 1)
						if (block(x, y, z - 1) == BlockID::Air)
						{
							vertices.push_back({{x + 1, y, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, block_data.texture[BLOCK_FACE_BACK]});
							vertices.push_back({{x, y, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, block_data.texture[BLOCK_FACE_BACK]});
							vertices.push_back({{x, y + 1, z}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, block_data.texture[BLOCK_FACE_BACK]});
							vertices.push_back({{x + 1, y + 1, z}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, block_data.texture[BLOCK_FACE_BACK]});
							ADD_INDEX
						}
					}
				}
			}
		}
	}

	void create()
	{
		static Timer timer; timer.start();

		glm::ivec3 size_block = size * CHUNK_SIZE;

		for (int x = 0; x < size_block.x; x++)
		{
			// right face
			createFace(
				{x, 0, 0},
				{1, size_block.y, size_block.z},
				{
					glm::ivec3{1, 0, 1},
					glm::ivec3{1, 0, 0},
					glm::ivec3{1, 1, 0},
					glm::ivec3{1, 1, 1}
				},
				{1, 0, 0},
				BLOCK_FACE_RIGHT
			);

			// left face
			createFace(
				{x, 0, 0},
				{1, size_block.y, size_block.z},
				{
					glm::ivec3{0, 0, 0},
					glm::ivec3{0, 0, 1},
					glm::ivec3{0, 1, 1},
					glm::ivec3{0, 1, 0}
				},
				{-1, 0, 0},
				BLOCK_FACE_LEFT
			);
		}

		for (int y = 0; y < size_block.y; y++)
		{
			// top face
			createFace(
				{0, y, 0},
				{size_block.x, 1, size_block.z},
				{
					glm::ivec3{1, 1, 0},
					glm::ivec3{0, 1, 0},
					glm::ivec3{0, 1, 1},
					glm::ivec3{1, 1, 1}
				},
				{0, 1, 0},
				BLOCK_FACE_TOP
			);

			// bottom face
			createFace(
				{0, y, 0},
				{size_block.x, 1, size_block.z},
				{
					glm::ivec3{0, 0, 0},
					glm::ivec3{1, 0, 0},
					glm::ivec3{1, 0, 1},
					glm::ivec3{0, 0, 1}
				},
				{0, -1, 0},
				BLOCK_FACE_BOTTOM
			);
		}

		for (int z = 0; z < size_block.z; z++)
		{
			// back face
			createFace(
				{0, 0, z},
				{size_block.x, size_block.y, 1},
				{
					glm::ivec3{0, 0, 1},
					glm::ivec3{1, 0, 1},
					glm::ivec3{1, 1, 1},
					glm::ivec3{0, 1, 1}
				},
				{0, 0, 1},
				BLOCK_FACE_BACK
			);

			// front face
			createFace(
				{0, 0, z},
				{size_block.x, size_block.y, 1},
				{
					glm::ivec3{1, 0, 0},
					glm::ivec3{0, 0, 0},
					glm::ivec3{0, 1, 0},
					glm::ivec3{1, 1, 0}
				},
				{0, 0, -1},
				BLOCK_FACE_FRONT
			);
		}
		timer.stop();
		DebugGui::create_mesh_time = timer.average<std::chrono::microseconds>();
	}

	/**
	 * @brief Create the mesh for a particular face for a specific range of blocks
	 * 
	 * @param start the starting position
	 * @param max_iter the maximum iteration for the x, y, z axis
	 * @param offsets the offsets for the 4 vertices of the face
	 * @param normal the normal of the face
	 * @param face the texture index for the face
	 */
	void createFace(
		const glm::ivec3 & start,
		const glm::ivec3 & max_iter,
		const std::array<glm::ivec3, 4> & offsets,
		const glm::ivec3 & normal,
		const int face
	)
	{
		glm::ivec3 final_max_iter = start + max_iter;
		for (int x = start.x; x < final_max_iter.x; x++)
		{
			for (int y = start.y; y < final_max_iter.y; y++)
			{
				for (int z = start.z; z < final_max_iter.z; z++)
				{
					BlockID block_id = block(x, y, z);

					if (block_id != BlockID::Air)
					{
						BlockID neighbor_id = block(x + normal.x, y + normal.y, z + normal.z);

						if (neighbor_id == BlockID::Air)
						{
							Data block_data = Block::getData(block_id);

							glm::ivec3 pos = {x, y, z};

							vertices.push_back({pos + offsets[0], normal, {0.0f, 1.0f}, block_data.texture[face]});
							vertices.push_back({pos + offsets[1], normal, {1.0f, 1.0f}, block_data.texture[face]});
							vertices.push_back({pos + offsets[2], normal, {1.0f, 0.0f}, block_data.texture[face]});
							vertices.push_back({pos + offsets[3], normal, {0.0f, 0.0f}, block_data.texture[face]});
							ADD_INDEX
						}

					}
				}
			}
		}
	}

	#undef ADD_INDEX

	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;
};
