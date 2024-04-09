#pragma once 
#include "define.hpp"

#include "Chunk.hpp"
#include "hashes.hpp"
#include <unordered_map>

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
};
