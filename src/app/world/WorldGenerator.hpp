#pragma once 

#include "define.hpp"

#include "Chunk.hpp"
#include "Block.hpp"

class WorldGenerator
{
public:
	WorldGenerator();
	~WorldGenerator();

	Chunk && generateChunk(const int & x, const int & y, const int & z);
private:
	
};
