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
	typedef std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> BlockArray;

	Block	getBlock(const int & x, const int & y, const int & z) const;
	void	setBlock(const int & x, const int & y, const int & z, Block block);

	static  int			toIndex(const int & x, const int & y, const int & z);
	static	glm::ivec3	toCoord(const int & index);
private:
	BlockArray	m_blocks;
	const		glm::ivec3 m_position;

};
