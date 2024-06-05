#pragma once 


#include "World.hpp"
#include "ThreadPool.hpp"

class ServerWorld : public World
{
public:
	ServerWorld();
	~ServerWorld();

	ServerWorld(ServerWorld & other) = delete;
	ServerWorld(ServerWorld && other) = delete;
	ServerWorld & operator=(ServerWorld & other) = delete;
	ServerWorld & operator=(ServerWorld && other) = delete;

	Chunk &			getChunk(const glm::ivec3 & chunk_position);
	const Chunk &	getChunk(const glm::ivec3 & chunk_position) const;
	
	Chunk & 		getAndLoadChunk(const glm::ivec3 & chunk_position);

	void			loadChunk(const glm::ivec3 & chunk_position);

	void			setBlock(const glm::vec3 & position, BlockID block);
private:

};
