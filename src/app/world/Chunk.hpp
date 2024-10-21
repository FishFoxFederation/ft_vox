#pragma once

#include "hashes.hpp"
#include <array>
#include <glm/vec3.hpp>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include "Block.hpp"
#include "Status.hpp"
#include "define.hpp"
#include "Tracy.hpp"

#define CHUNK_Y_SIZE 512
#define CHUNK_X_SIZE 16
#define CHUNK_Z_SIZE 16
#define BLOCKS_PER_CHUNK CHUNK_Y_SIZE * CHUNK_X_SIZE * CHUNK_Z_SIZE
#define CHUNK_SIZE_IVEC3 glm::ivec3(CHUNK_X_SIZE, CHUNK_Y_SIZE, CHUNK_Z_SIZE)
#define CHUNK_SIZE_VEC3 glm::vec3(CHUNK_X_SIZE, CHUNK_Y_SIZE, CHUNK_Z_SIZE)

glm::ivec3 getChunkPos(const glm::ivec3 & block_pos);
glm::ivec3 getBlockChunkPos(const glm::ivec3 & block_pos);

class Chunk
{
public:
	typedef std::array<BlockInfo::Type, BLOCKS_PER_CHUNK> BlockArray;
	typedef std::array<uint8_t, BLOCKS_PER_CHUNK> LightArray;
	enum class genLevel : uint16_t
	{
		LIGHT,
		CAVE,
		RELIEF,
		EMPTY,
	};

	Chunk(glm::ivec3 position);
	// Chunk(const glm::ivec3 & position, const BlockArray & blocks);
	Chunk(const glm::ivec3 & position, const BlockArray & blocks, const LightArray & light);

	Chunk(const Chunk & other) = delete;
	Chunk(Chunk && other) = delete;
	Chunk & operator=(const Chunk & other) = delete;
	Chunk & operator=(const Chunk && other) = delete;
	~Chunk();

	BlockArray &		getBlocks();
	const BlockArray &	getBlocks() const;
	BlockInfo::Type				getBlock(const int & x, const int & y, const int & z) const;
	BlockInfo::Type				getBlock(const glm::ivec3 & position) const;
	void				setBlock(const int & x, const int & y, const int & z, BlockInfo::Type block);
	void 				setBlock(const glm::ivec3 & position, BlockInfo::Type block);

	LightArray &		getLight();
	const LightArray &	getLight() const;
	uint8_t				getLight(const int & x, const int & y, const int & z) const;
	uint8_t				getLight(const glm::ivec3 & position) const;
	void				setLight(const int & x, const int & y, const int & z, uint8_t light);
	void 				setLight(const glm::ivec3 & position, uint8_t light);

	uint8_t				getSkyLight(const int & x, const int & y, const int & z) const;
	uint8_t				getSkyLight(const glm::ivec3 & position) const;
	void				setSkyLight(const int & x, const int & y, const int & z, uint8_t light);
	void 				setSkyLight(const glm::ivec3 & position, uint8_t light);

	uint8_t				getBlockLight(const int & x, const int & y, const int & z) const;
	uint8_t				getBlockLight(const glm::ivec3 & position) const;
	void				setBlockLight(const int & x, const int & y, const int & z, uint8_t light);
	void 				setBlockLight(const glm::ivec3 & position, uint8_t light);

	const glm::ivec3 &	getPosition() const;
	void 				setPosition(const glm::ivec3 & position);

	const int & 		x()const {return position.x;};
	const int & 		y()const {return position.y;};
	const int & 		z()const {return position.z;};

	uint64_t			getMeshID() const;
	void				setMeshID(const uint64_t & mesh_id);

	bool				isMeshed() const;
	void				setMeshed(bool meshed);

	int					getLoadLevel() const;
	int					getHighestLoadLevel() const;
	void				setLoadLevel(const int & load_level);

	genLevel			getGenLevel() const;
	void				setGenLevel(const genLevel & level);

	static  int			toIndex(const int & x, const int & y, const int & z);
	static	glm::ivec3	toCoord(const int & index);


	// TracySharedLockableN			(std::shared_mutex,	, "Chunk Status");
	Status 							status;
	std::unordered_set<uint64_t>	entity_ids;
	std::unordered_set<uint64_t>	observing_player_ids;
private:
	std::atomic<bool> 		meshed = false;
	glm::ivec3	position;
	std::atomic<uint64_t>	m_mesh_id;
	mutable TracyLockableN	(std::mutex, m_mesh_id_mutex, "Mesh ID");
	BlockArray	m_blocks;
	// 4 left bits for block light, 4 right bits for sky light
	LightArray	m_light;
	int			load_level = TICKET_LEVEL_INACTIVE + 10;
	int			highest_load_level = 0;
	genLevel	m_gen_level = genLevel::EMPTY;
};

typedef std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> ChunkMap;

namespace std
{
	template<>
	struct hash<Chunk::genLevel>
	{
		std::size_t operator()(const Chunk::genLevel & level) const
		{
			return std::hash<uint16_t>()(static_cast<uint16_t>(level));
		}
	};

	template <>
	struct hash<std::pair<glm::ivec3, Chunk::genLevel>>
	{
		std::size_t operator()(const std::pair<glm::ivec3, Chunk::genLevel> & pair) const
		{
			return std::hash<glm::ivec3>()(pair.first) ^ std::hash<Chunk::genLevel>()(pair.second);
		}
	};
}
