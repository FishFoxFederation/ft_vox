#pragma once


#include <array>
#include <glm/vec3.hpp>
#include <mutex>
#include <condition_variable>
#include "Block.hpp"
#include "define.hpp"

#define CHUNK_Y_SIZE 256
#define CHUNK_X_SIZE 16
#define CHUNK_Z_SIZE 16
#define BLOCKS_PER_CHUNK CHUNK_Y_SIZE * CHUNK_X_SIZE * CHUNK_Z_SIZE
#define CHUNK_SIZE_IVEC3 glm::ivec3(CHUNK_X_SIZE, CHUNK_Y_SIZE, CHUNK_Z_SIZE)


class Chunk
{
public:
	class ChunkStatus
	{
	public:
		
		const static inline uint64_t CLEAR = 0;
		const static inline uint64_t WORKING = 1 << 1;
		const static inline uint64_t MESHING = 1 << 2;
		const static inline uint64_t DELETING = 1 << 3;

		ChunkStatus()
			: m_flags(CLEAR), m_counter(0)
		{};
		~ChunkStatus(){};

		void			waitForClear();
		void			setFlag(const uint64_t & flag);
		void 			clearFlag(const uint64_t & flag);
		void			addWorking();
		void			removeWorking();
		bool			isSet(const uint64_t & flag);
		bool			isClear();
		std::mutex &	getMutex() {return m_mutex;};
	private:
		uint64_t				m_flags;
		uint64_t				m_counter;
		std::mutex				m_mutex;
		std::condition_variable m_cv;
	};

	Chunk(glm::ivec3 position);

	// Chunk(const Chunk & other);
	Chunk & operator=(const Chunk && other);
	Chunk(Chunk && other);
	~Chunk();

	typedef std::array<BlockID, BLOCKS_PER_CHUNK> BlockArray;

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

	ChunkStatus	status;
private:
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;
};
