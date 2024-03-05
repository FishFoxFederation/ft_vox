#pragma once


#include <array>
#include <glm/vec3.hpp>
#include "Block.hpp"
#include "define.hpp"

#define CHUNK_SIZE 16

class Chunk
{
public:
	Chunk(glm::ivec3 position);

	Chunk(const Chunk & other);
	Chunk & operator=(const Chunk & other);
	Chunk(Chunk && other);
	~Chunk();

	typedef std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> BlockArray;

	Block				getBlock(const int & x, const int & y, const int & z) const;
	void				setBlock(const int & x, const int & y, const int & z, Block block);

	const uint64_t	&	getMeshID() const;
	void				setMeshID(const uint64_t & mesh_id);

	static  int			toIndex(const int & x, const int & y, const int & z);
	static	glm::ivec3	toCoord(const int & index);
private:
	const		glm::ivec3 m_position;
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;

};
