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

/**
 * @class Chunk
 * @brief a class containing a 3D array of blocks
 * @details This class is a container for a 3D array of blocks, it contains its own position
 * and its mesh id if it has been meshed, it also contains a ChunkStatus object to manage the asyncronous operations performed on it.
 * The size of the chunk is defined by the CHUNK_X_SIZE, CHUNK_Y_SIZE and CHUNK_Z_SIZE macros.
 * The blocks are stored in a 1D array so some math shenanigans are used to access them from 3D coordinates.
 * 
 */
class Chunk
{
public:
	/**
	 * @brief a class Managing asyncronous chunk status
	 * @details This class is here to be able to perform safely asyncronous operations on the chunk.
	 * It contains a flag system to know the status of the chunk and a counter to know how many threads are working on it.
	 * The logic concercning the flags is the following:
	 * - CLEAR: the chunk is ready to be worked on
	 * - WORKING: a thread is working on the chunk, you cannot delete it or modify it
	 * - MESHING: a thread is meshing the chunk, you cannot delete it or modify it
	 * - DELETING: a thread is deleting the chunk, you cannot do anything 
	 * - LOADING: a thread is loading the chunk, not read ready nor write ready
	 * @warning As of now, it is not possible to modify the blocks safely, only read them and mesh the chunk
	 * @note For future afaure, it would be nice to have some sort of soft locks and read/write ready flags
	 */
	class ChunkStatus
	{
	public:
		
		const static inline uint64_t CLEAR = 0;
		const static inline uint64_t WORKING = 1 << 1;
		const static inline uint64_t MESHING = 1 << 2;
		const static inline uint64_t DELETING = 1 << 3;
		const static inline uint64_t LOADING = 1 << 4;

		ChunkStatus()
			: m_flags(CLEAR), m_counter(0)
		{};
		~ChunkStatus(){};

		/**
		 * @brief Will block until the chunk's status is CLEAR
		 */
		void			waitForClear();

		/**
		 * @brief Add a flag to the current status
		 * 
		 * @param flag one of the Macros defined in ChunkStatus
		 */
		void			setFlag(const uint64_t & flag);

		/**
		 * @brief Remove a flag from the current status
		 * 
		 * @param flag one of the Macros defined in ChunkStatus
		 */
		void 			clearFlag(const uint64_t & flag);

		/**
		 * @brief To call when a thread is working on the chunk 
		 * @note ONLY read operations are allowed for now
		 */
		void			addWorking();

		/**
		 * @brief To call when a thread is done working on the chunk
		 */
		void			removeWorking();

		/**
		 * @brief Check if a flag is set
		 * 
		 * @param flag one of the Macros defined in ChunkStatus
		 * @return true if the flag is set
		 */
		bool			isSet(const uint64_t & flag);

		/**
		 * @brief Check if the chunk is clear
		 * @return true if the chunk is clear
		 */
		bool			isClear();

		/**
		 * @brief Expose the underlying mutex
		 * @warning be very careful with this pls i dont want deadlocks ;-;
		 * @return std::mutex& 
		 */
		std::mutex &	getMutex() {return m_mutex;};
	private:
		uint64_t				m_flags;
		uint64_t				m_counter;
		std::mutex				m_mutex;
		std::condition_variable m_cv;
	};

	/*************************************************************
	 * CONSTRUCTORS
	**************************************************************/

	Chunk(glm::ivec3 position);

	Chunk(const Chunk & other) = delete;
	Chunk & operator=(const Chunk & other) = delete;

	Chunk & operator=(const Chunk && other);
	Chunk(Chunk && other);
	~Chunk();

	/*************************************************************
	 * DEFINES
	**************************************************************/

	typedef std::array<BlockID, BLOCKS_PER_CHUNK> BlockArray;


	/*************************************************************
	 * METHODS
	 * *******************************************************/

	/**
	 * @brief Get the Block at the given chunk-relative position
	 * 
	 * @param position a 3D glm integer vector
	 * @return BlockID
	 */
	BlockID				getBlock(const glm::ivec3 & position) const;

	/**
	 * @brief Set the Block at the given chunk-relative position
	 * 
	 * @param positiion a 3D glm integer vector
	 * @param block the block to place
	 */
	void				setBlock(const glm::ivec3 & position, BlockID block);

	/**
	 * @brief Get the positition of the chunk in the world
	 * 
	 * @return a 3D glm integer vector
	 */
	const glm::ivec3 &	getPosition() const {return position;};

	/** @brief Get the x position of the chunk in the world */
	const int & 		x()const {return position.x;};
	/** @brief Get the y position of the chunk in the world */
	const int & 		y()const {return position.y;};
	/** @brief Get the z position of the chunk in the world */
	const int & 		z()const {return position.z;};

	/**
	 * @brief Get the Mesh ID of the chunk
	 * 
	 * @return the curent mesh_id, could be 0 if the chunk has not been meshed 
	 */
	const uint64_t	&	getMeshID() const;

	/**
	 * @brief Set the Mesh ID of the chunk
	 * 
	 * @param mesh_id the new mesh_id
	 */
	void				setMeshID(const uint64_t & mesh_id);

	/**
	 * @brief transpose a 3D position to a 1D index, more info in the implementation
	 * 
	 * @param position
	 * @return int 
	 */
	static  int			toIndex(const glm::ivec3 & position);

	/**
	 * @brief transpose a 1D index to a 3D position, more info in the implementation
	 * 
	 * @param index 
	 * @return glm::ivec3 
	 */
	static	glm::ivec3	toCoord(const int & index);

	const		glm::ivec3 position;

	ChunkStatus	status;
private:
	uint64_t	m_mesh_id;
	BlockArray	m_blocks;
};
