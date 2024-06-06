#pragma once 


#include "World.hpp"
#include "ThreadPool.hpp"
#include "server_define.hpp"
#include <unordered_set>

class ServerWorld : public World
{
public:
	ServerWorld();
	~ServerWorld();

	ServerWorld(ServerWorld & other) = delete;
	ServerWorld(ServerWorld && other) = delete;
	ServerWorld & operator=(ServerWorld & other) = delete;
	ServerWorld & operator=(ServerWorld && other) = delete;

	struct ChunkLoadUnloadData
	{
		std::vector<std::shared_ptr<Chunk>>	chunks_to_load;
		std::vector<glm::ivec3>				chunks_to_unload;
	};

	std::shared_ptr<Chunk>	getAndLoadChunk(const glm::ivec3 & chunk_position);

	void					loadChunk(const glm::ivec3 & chunk_position);

	void					setBlock(const glm::vec3 & position, BlockID block);

	ChunkLoadUnloadData			getChunksToUnload(
		const glm::vec3 & old_player_position,
		const glm::vec3 & new_player_position);
private:

};
