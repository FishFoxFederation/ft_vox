#pragma once

#include "define.hpp"
#include "vk_define.hpp"
#include "DebugGui.hpp"
#include "Timer.hpp"
#include "logger.hpp"

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

class CreateMeshData
{

public:

	/**
	 * @brief Create a Mesh Data object
	 *
	 * @param pos the position of the chunk in the chunk map
	 * @param size the number of chunks to create the mesh data
	 * @param chunk_map the chunk map
	 */
	CreateMeshData(const glm::ivec3 & pos, const glm::ivec3 & size, ChunkMap & chunk_map):
		chunks(size.x + 2, std::vector<std::vector<Chunk *>>(size.y + 2, std::vector<Chunk *>(size.z + 2, nullptr))),
		face_data(
			size.x * CHUNK_SIZE,
			std::vector<std::vector<FaceData>>(
				size.y * CHUNK_SIZE,
				std::vector<FaceData>(size.z * CHUNK_SIZE, {0, 0})
			)
		),
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
					BlockID neighbor_id = block(x + normal.x, y + normal.y, z + normal.z);

					if (block_id != BlockID::Air && neighbor_id == BlockID::Air)
					{
						face_data[x][y][z] = {Block::getData(block_id).texture[face], 0};
					}
					else
					{
						face_data[x][y][z] = {0, 0};
					}
				}
			}
		}

		for (int x = start.x; x < final_max_iter.x; x++)
		{
			for (int y = start.y; y < final_max_iter.y; y++)
			{
				for (int z = start.z; z < final_max_iter.z; z++)
				{
					FaceData data = face_data[x][y][z];

					if (data.texture != 0)
					{
						int merge_count = 0;
						// check if the block has identical neighbors for greedy meshing
						// if so, then merge the blocks into one mesh
						glm::ivec3 offset{0, 0, 0};
						glm::ivec3 saved_offset{0, 0, 0};
						for (offset.x = x; offset.x < final_max_iter.x; offset.x++)
						{
							for (offset.y = y; offset.y < final_max_iter.y && (saved_offset.y == 0 || offset.y < saved_offset.y); offset.y++)
							{
								// continue if still in chunk bounds and if either it's the first iteration or the offset is less than the saved offset
								for (offset.z = z; offset.z < final_max_iter.z && (saved_offset.z == 0 || offset.z < saved_offset.z); offset.z++)
								{
									if (face_data[offset.x][offset.y][offset.z] != data)
									{
										break;
									}
									merge_count++;
								}
								// save the offset if it's the first iteration
								if (saved_offset.z == 0)
								{
									saved_offset.z = offset.z;
								}
								// if the offset is different than the saved offset, then break
								else if (offset.z != saved_offset.z)
								{
									break;
								}
							}
							// save the offset if it's the first iteration
							if (saved_offset.y == 0)
							{
								saved_offset.y = offset.y;
							}
							// if the offset is different than the saved offset, then break
							else if (offset.y != saved_offset.y)
							{
								break;
							}
						}
						saved_offset.x = offset.x;

						for (offset.x = x; offset.x < saved_offset.x; offset.x++)
						{
							for (offset.y = y; offset.y < saved_offset.y; offset.y++)
							{
								for (offset.z = z; offset.z < saved_offset.z; offset.z++)
								{
									face_data[offset.x][offset.y][offset.z] = {0, 0};
								}
							}
						}

						saved_offset -= glm::ivec3(x, y, z);

						if (normal.x + normal.y + normal.z < 0)
						{
							saved_offset += normal;
						}

						glm::vec2 texCoord = {0.0f, 0.0f};
						if (normal.x != 0)
						{
							texCoord = {saved_offset.z, saved_offset.y};
						}
						else if (normal.y != 0)
						{
							texCoord = {saved_offset.x, saved_offset.z};
						}
						else if (normal.z != 0)
						{
							texCoord = {saved_offset.x, saved_offset.y};
						}
						// texCoord = {1.0f, 1.0f};

						vertices.push_back({{x + offsets[0].x * saved_offset.x, y + offsets[0].y * saved_offset.y, z + offsets[0].z * saved_offset.z}, normal, {0.0f * texCoord.x, 1.0f * texCoord.y}, data.texture});
						vertices.push_back({{x + offsets[1].x * saved_offset.x, y + offsets[1].y * saved_offset.y, z + offsets[1].z * saved_offset.z}, normal, {1.0f * texCoord.x, 1.0f * texCoord.y}, data.texture});
						vertices.push_back({{x + offsets[2].x * saved_offset.x, y + offsets[2].y * saved_offset.y, z + offsets[2].z * saved_offset.z}, normal, {1.0f * texCoord.x, 0.0f * texCoord.y}, data.texture});
						vertices.push_back({{x + offsets[3].x * saved_offset.x, y + offsets[3].y * saved_offset.y, z + offsets[3].z * saved_offset.z}, normal, {0.0f * texCoord.x, 0.0f * texCoord.y}, data.texture});

						indices.push_back(vertices.size() - 4);
						indices.push_back(vertices.size() - 3);
						indices.push_back(vertices.size() - 2);
						indices.push_back(vertices.size() - 4);
						indices.push_back(vertices.size() - 2);
						indices.push_back(vertices.size() - 1);

					}
				}
			}
		}
	}

	std::vector<std::vector<std::vector<Chunk *>>> chunks;
	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;

private:

	struct FaceData
	{
		TextureID texture;
		int ao;

		bool operator==(const FaceData & other) const
		{
			return texture == other.texture && ao == other.ao;
		}
	};

	std::vector<std::vector<std::vector<FaceData>>> face_data;
	glm::ivec3 size;

};
