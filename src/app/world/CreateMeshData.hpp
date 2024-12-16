#pragma once

#include "define.hpp"
#include "vk_define.hpp"
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
	uint8_t ao;

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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

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

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R8_UINT;
		attributeDescriptions[4].offset = offsetof(BlockVertex, ao);

		return attributeDescriptions;
	}

	bool operator==(const BlockVertex& other) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord && texLayer == other.texLayer && ao == other.ao;
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
			size.x * CHUNK_X_SIZE,
			std::vector<std::vector<FaceData>>(
				size.y * CHUNK_Y_SIZE,
				std::vector<FaceData>(size.z * CHUNK_Z_SIZE, {0, 0})
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
					{
						chunks[x + 1][y + 1][z + 1] = &it->second;
						it->second.status.addReader();
					}
				}
			}
		}
	}

	CreateMeshData(const CreateMeshData &) = delete;
	CreateMeshData & operator=(const CreateMeshData &) = delete;

	CreateMeshData(CreateMeshData && other)
	:
		chunks(std::move(other.chunks)),
		vertices(std::move(other.vertices)),
		indices(std::move(other.indices)),
		face_data(std::move(other.face_data)),
		size(std::move(other.size))
	{
		// other.chunks.clear();
		// other.vertices.clear();
		// other.indices.clear();
		// other.face_data.clear();
	}

	~CreateMeshData()
	{
		if (chunks.empty())
			return;

		for (int x = 0; x < size.x + 2; x++)
		{
			for (int y = 0; y < size.y + 2; y++)
			{
				for (int z = 0; z < size.z + 2; z++)
				{
					if (chunks[x][y][z] != nullptr)
					{
						chunks[x][y][z]->status.removeReader();
					}
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

	enum class Dimensions
	{
		X = 0,
		Y = 1,
		Z = 2
	};

	BlockID block(const int x, const int y, const int z)
	{
		const int chunk_x = (x + CHUNK_X_SIZE) / CHUNK_X_SIZE;
		const int chunk_y = (y + CHUNK_Y_SIZE) / CHUNK_Y_SIZE;
		const int chunk_z = (z + CHUNK_Z_SIZE) / CHUNK_Z_SIZE;

		const int block_x = (x + CHUNK_X_SIZE) % CHUNK_X_SIZE;
		const int block_y = (y + CHUNK_Y_SIZE) % CHUNK_Y_SIZE;
		const int block_z = (z + CHUNK_Z_SIZE) % CHUNK_Z_SIZE;

		if (chunks[chunk_x][chunk_y][chunk_z] == nullptr)
		{
			return BlockID::Air;
		}

		return chunks[chunk_x][chunk_y][chunk_z]->getBlock(block_x, block_y, block_z);
	}

	BlockID block(const glm::ivec3 & pos)
	{
		return block(pos.x, pos.y, pos.z);
	}

	void create()
	{
		glm::ivec3 size_block = size * CHUNK_SIZE_IVEC3;

		for (int x = 0; x < size_block.x; x++)
		{
			// right face
			createFace(
				static_cast<int>(Dimensions::Z),
				static_cast<int>(Dimensions::Y),
				{x, 0, 0},
				{1, size_block.y, size_block.z},
				{1, 0, 2, 1, 2, 3},
				{0, 2, 3, 0, 3, 1},
				{1, 0, 0}, 1,
				BLOCK_FACE_RIGHT
			);

			// left face
			createFace(
				static_cast<int>(Dimensions::Z),
				static_cast<int>(Dimensions::Y),
				{x, 0, 0},
				{1, size_block.y, size_block.z},
				{0, 1, 2, 1, 3, 2},
				{0, 1, 3, 0, 3, 2},
				{1, 0, 0}, -1,
				BLOCK_FACE_LEFT
			);
		}

		for (int y = 0; y < size_block.y; y++)
		{
			// top face
			createFace(
				static_cast<int>(Dimensions::X),
				static_cast<int>(Dimensions::Z),
				{0, y, 0},
				{size_block.x, 1, size_block.z},
				{1, 0, 2, 1, 2, 3},
				{0, 2, 3, 0, 3, 1},
				{0, 1, 0}, 1,
				BLOCK_FACE_TOP
			);

			// bottom face
			createFace(
				static_cast<int>(Dimensions::X),
				static_cast<int>(Dimensions::Z),
				{0, y, 0},
				{size_block.x, 1, size_block.z},
				{0, 1, 2, 1, 3, 2},
				{0, 1, 3, 0, 3, 2},
				{0, 1, 0}, -1,
				BLOCK_FACE_BOTTOM
			);
		}

		for (int z = 0; z < size_block.z; z++)
		{
			// front face
			createFace(
				static_cast<int>(Dimensions::X),
				static_cast<int>(Dimensions::Y),
				{0, 0, z},
				{size_block.x, size_block.y, 1},
				{0, 1, 2, 1, 3, 2},
				{0, 1, 3, 0, 3, 2},
				{0, 0, 1}, 1,
				BLOCK_FACE_FRONT
			);

			// back face
			createFace(
				static_cast<int>(Dimensions::X),
				static_cast<int>(Dimensions::Y),
				{0, 0, z},
				{size_block.x, size_block.y, 1},
				{1, 0, 2, 1, 2, 3},
				{0, 2, 3, 0, 3, 1},
				{0, 0, 1}, -1,
				BLOCK_FACE_BACK
			);
		}
	}

	void createFace(
		const int dim_1,
		const int dim_2,
		const glm::ivec3 & start,
		const glm::ivec3 & max_iter,
		const std::array<int, 6> & indices_order,
		const std::array<int, 6> & indices_order_fliped,
		const glm::ivec3 & abs_normal,
		const int normal_signe,
		const int face
	)
	{
		const glm::ivec3 normal = abs_normal * normal_signe;

		glm::ivec3 tmp = normal_signe > 0 ? normal : glm::ivec3{0, 0, 0};
		std::array<glm::ivec3, 4> offsets = { tmp, tmp, tmp, tmp };
		offsets[1][dim_1] = 1;
		offsets[2][dim_2] = 1;
		offsets[3][dim_1] = 1;
		offsets[3][dim_2] = 1;

		std::array<glm::vec2, 4> tex_coord_factor = {
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.0f, 1.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(0.0f, 0.0f)
		};

		if (face == BLOCK_FACE_LEFT || face == BLOCK_FACE_FRONT || face == BLOCK_FACE_BOTTOM)
		{
			tex_coord_factor = {
				glm::vec2(0.0f, 1.0f),
				glm::vec2(1.0f, 1.0f),
				glm::vec2(0.0f, 0.0f),
				glm::vec2(1.0f, 0.0f)
			};
		}

		glm::ivec3 final_max_iter = start + max_iter;
		glm::ivec3 pos;
		for (pos.x = start.x; pos.x < final_max_iter.x; pos.x++)
		{
			for (pos.y = start.y; pos.y < final_max_iter.y; pos.y++)
			{
				for (pos.z = start.z; pos.z < final_max_iter.z; pos.z++)
				{
					BlockID block_id = block(pos.x, pos.y, pos.z);
					BlockID neighbor_id = block(pos.x + normal.x, pos.y + normal.y, pos.z + normal.z);

					if (block_id != BlockID::Air && neighbor_id == BlockID::Air)
					{
						face_data[pos.x][pos.y][pos.z] = {Block::getData(block_id).texture[face], getAmbientOcclusion(pos + normal, dim_1, dim_2)};
					}
					else
					{
						face_data[pos.x][pos.y][pos.z] = {0, 0};
					}
				}
			}
		}

		for (pos.x = start.x; pos.x < final_max_iter.x; pos.x++)
		{
			for (pos.y = start.y; pos.y < final_max_iter.y; pos.y++)
			{
				for (pos.z = start.z; pos.z < final_max_iter.z; pos.z++)
				{
					FaceData data = face_data[pos.x][pos.y][pos.z];

					if (data.texture != 0)
					{
						int merge_count = 0;
						// check if the block has identical neighbors for greedy meshing
						// if so, then merge the blocks into one mesh
						glm::ivec3 offset{0, 0, 0};
						glm::ivec3 saved_offset{0, 0, 0};
						for (offset.x = pos.x; offset.x < final_max_iter.x; offset.x++)
						{
							for (offset.y = pos.y; offset.y < final_max_iter.y && (saved_offset.y == 0 || offset.y < saved_offset.y); offset.y++)
							{
								// continue if still in chunk bounds and if either it's the first iteration or the offset is less than the saved offset
								for (offset.z = pos.z; offset.z < final_max_iter.z && (saved_offset.z == 0 || offset.z < saved_offset.z); offset.z++)
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

						// saved_offset = glm::ivec3(pos.x + 1, pos.y + 1, pos.z + 1);

						for (offset.x = pos.x; offset.x < saved_offset.x; offset.x++)
						{
							for (offset.y = pos.y; offset.y < saved_offset.y; offset.y++)
							{
								for (offset.z = pos.z; offset.z < saved_offset.z; offset.z++)
								{
									face_data[offset.x][offset.y][offset.z] = {0, 0};
								}
							}
						}

						saved_offset -= pos;

						if (normal.x + normal.y + normal.z < 0)
						{
							saved_offset += normal;
						}

						glm::vec2 tex_coord = { saved_offset[dim_1], saved_offset[dim_2] };
						// tex_coord = {1.0f, 1.0f};

						for (int i = 0; i < 4; i++)
						{
							vertices.push_back({pos + offsets[i] * saved_offset, normal, tex_coord_factor[i] * tex_coord, data.texture, data.ao[i]});
						}

						if (data.ao[0] + data.ao[3] > data.ao[1] + data.ao[2]) // if the first triangle has more ambient occlusion than the second triangle
						{
							for (int i = 0; i < 6; i++)
							{
								indices.push_back(vertices.size() - 4 + indices_order[i]);
							}
						}
						else
						{
							for (int i = 0; i < 6; i++)
							{
								indices.push_back(vertices.size() - 4 + indices_order_fliped[i]);
							}
						}
					}
				}
			}
		}
	}

	std::array<uint8_t, 4> getAmbientOcclusion(
		const glm::ivec3 & pos,
		const int dim_1,
		const int dim_2
	)
	{
		std::array<uint8_t, 4> ao = {0, 0, 0, 0};

		glm::ivec3 side_1 = pos;
		glm::ivec3 side_2 = pos;
		glm::ivec3 corner = pos;

		side_1[dim_1]--;
		side_2[dim_2]--;
		corner[dim_1]--;
		corner[dim_2]--;
		ao[0] = getAmbientOcclusion(block(side_1), block(side_2), block(corner));

		side_1 = pos;
		side_2 = pos;
		corner = pos;

		side_1[dim_1]++;
		side_2[dim_2]--;
		corner[dim_1]++;
		corner[dim_2]--;
		ao[1] = getAmbientOcclusion(block(side_1), block(side_2), block(corner));

		side_1 = pos;
		side_2 = pos;
		corner = pos;

		side_1[dim_1]--;
		side_2[dim_2]++;
		corner[dim_1]--;
		corner[dim_2]++;
		ao[2] = getAmbientOcclusion(block(side_1), block(side_2), block(corner));

		side_1 = pos;
		side_2 = pos;
		corner = pos;

		side_1[dim_1]++;
		side_2[dim_2]++;
		corner[dim_1]++;
		corner[dim_2]++;
		ao[3] = getAmbientOcclusion(block(side_1), block(side_2), block(corner));

		return ao;
	}

	int getAmbientOcclusion(
		BlockID side_1,
		BlockID side_2,
		BlockID corner
	)
	{
		return Block::hasProperty(side_1, BLOCK_PROPERTY_OPAQUE | BLOCK_PROPERTY_SOLID) +
				Block::hasProperty(side_2, BLOCK_PROPERTY_OPAQUE | BLOCK_PROPERTY_SOLID) +
				Block::hasProperty(corner, BLOCK_PROPERTY_OPAQUE | BLOCK_PROPERTY_SOLID);
	}

	std::vector<std::vector<std::vector<Chunk *>>> chunks;
	std::vector<BlockVertex> vertices;
	std::vector<uint32_t> indices;

	Chunk * getCenterChunk()
	{
		return chunks[NEUT][NEUT][NEUT];
	}

private:

	struct FaceData
	{
		TextureID texture;
		std::array<uint8_t, 4> ao;

		bool operator==(const FaceData & other) const
		{
			return texture == other.texture && ao == other.ao;
		}
	};

	std::vector<std::vector<std::vector<FaceData>>> face_data;
	glm::ivec3 size;

};
