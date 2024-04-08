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

	typedef std::array<BlockID, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> BlockArray;

	BlockID				getBlock(const int & x, const int & y, const int & z) const;
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
private:
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;

};
