#pragma once


#include <array>
#include <glm/vec3.hpp>
#include <mutex>
#include <condition_variable>
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


class Chunk
{
public:
	typedef std::array<BlockID, BLOCKS_PER_CHUNK> BlockArray;
	typedef std::array<uint8_t, BLOCKS_PER_CHUNK> LightArray;
	enum class genLevel : int
	{
		CAVE,
		RELIEF,
		EMPTY,
	};	

	Chunk(glm::ivec3 position);
	Chunk(const glm::ivec3 & position, const BlockArray & blocks);
	Chunk(const glm::ivec3 & position, const BlockArray & blocks, const LightArray & light);

	Chunk(const Chunk & other);
	Chunk & operator=(const Chunk & other);
	Chunk & operator=(const Chunk && other);
	Chunk(Chunk && other);
	~Chunk();

	BlockArray &		getBlocks();
	const BlockArray &	getBlocks() const;
	BlockID				getBlock(const int & x, const int & y, const int & z) const;
	BlockID				getBlock(const glm::ivec3 & position) const;
	void				setBlock(const int & x, const int & y, const int & z, BlockID block);
	void 				setBlock(const glm::ivec3 & position, BlockID block);

	LightArray &		getLight();
	const LightArray &	getLight() const;
	uint8_t				getLight(const int & x, const int & y, const int & z) const;
	uint8_t				getLight(const glm::ivec3 & position) const;
	void				setLight(const int & x, const int & y, const int & z, uint8_t light);
	void 				setLight(const glm::ivec3 & position, uint8_t light);

	const glm::ivec3 &	getPosition() const;
	void 				setPosition(const glm::ivec3 & position);

	const int & 		x()const {return position.x;};
	const int & 		y()const {return position.y;};
	const int & 		z()const {return position.z;};

	const uint64_t	&	getMeshID() const;
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
	bool 		meshed = false;
	glm::ivec3	position;
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;
	LightArray	m_light;
	int			load_level = 44;
	int			load_level = TICKET_LEVEL_INACTIVE + 10;
	int			highest_load_level = 0;
	genLevel	m_gen_level = genLevel::EMPTY;
};

typedef std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> ChunkMap;
