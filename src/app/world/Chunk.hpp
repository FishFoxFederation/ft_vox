#pragma once


#include <array>
#include <glm/vec3.hpp>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include "Block.hpp"
#include "Status.hpp"
#include "define.hpp"

#define CHUNK_Y_SIZE 256
#define CHUNK_X_SIZE 16
#define CHUNK_Z_SIZE 16
#define BLOCKS_PER_CHUNK CHUNK_Y_SIZE * CHUNK_X_SIZE * CHUNK_Z_SIZE
#define CHUNK_SIZE_IVEC3 glm::ivec3(CHUNK_X_SIZE, CHUNK_Y_SIZE, CHUNK_Z_SIZE)
#define CHUNK_SIZE_VEC3 glm::vec3(CHUNK_X_SIZE, CHUNK_Y_SIZE, CHUNK_Z_SIZE)


class Chunk
{
public:

	Chunk(glm::ivec3 position);

	Chunk(const Chunk & other) = delete;
	Chunk & operator=(const Chunk & other) = delete;
	Chunk & operator=(const Chunk && other);
	Chunk(Chunk && other);
	~Chunk();

	typedef std::array<BlockID, BLOCKS_PER_CHUNK> BlockArray;

	BlockID				getBlock(const int & x, const int & y, const int & z) const;
	BlockID				getBlock(const glm::vec3 & position) const;
	void				setBlock(const int & x, const int & y, const int & z, BlockID block);

	const glm::ivec3 &	getPosition() const {return position;};

	const int & 		x()const {return position.x;};
	const int & 		y()const {return position.y;};
	const int & 		z()const {return position.z;};


	const uint64_t	&	getMeshID() const;
	void				setMeshID(const uint64_t & mesh_id);

	static  int			toIndex(const int & x, const int & y, const int & z);
	static	glm::ivec3	toCoord(const int & index);

	const		glm::ivec3 position;

	Status	status;
	std::unordered_set<uint64_t>	entity_ids;
private:
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;
};
