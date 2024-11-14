#pragma once

#include <filesystem>
#include <fstream>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

#include "logger.hpp"
#include "Chunk.hpp"
#include "glm/glm.hpp"
#include "hashes.hpp"

/**
 * @brief a class to save and load chunks from the disk
 */
class Save
{
public:
	constexpr static int REGION_SIZE = 32;
	static const std::filesystem::path SAVE_DIR;
	static const std::filesystem::path DEFAULT_NAME;
	static glm::ivec2 toRegionPos(glm::ivec3 chunkPos3D);
	Save();
	Save(const std::filesystem::path & path);
	~Save();

	/**
	 * @brief add chunk to the list of chunks to save
	 * 
	 * @note you dont have to call trackChunk for chunks that you got from getChunk
	 * 
	 * @param chunk 
	 */
	void trackChunk(std::shared_ptr<Chunk> chunk);
	
	/**
	 * @brief Unloads the region and saves it to the disk.
	 * 
	 * @warning You SHOULD NOT have any references left to the chunks in the region,
	 * you take the risk of losing any changes made to the chunks as they will be untracked.
	 * 
	 * @param position 
	 */
	void saveRegion(const glm::ivec2 & position);

	/**
	 * @brief Tries to load a chunk from the disk
	 * 
	 * @param position position of the chunk
	 * @retval nullptr if the chunk is not found
	 * @retval std::shared_ptr<Chunk> if the chunk is found
	 */
	std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & position);
private:
	std::filesystem::path m_save_dir;

	/**
	 * @brief A class representing a region of the save
	 * 
	 * @details
	 * Will read the file and load all the chunks in the region when you request one chunk.
	 * 
	 * Will save all the chunks in the region and erase them from the internal list when you call save.
	 */
	class Region
	{
	public:
		class CorruptedFileException : public std::exception
		{
		public:
			CorruptedFileException(const std::string & message)
				: m_message(message) {}
			const char * what() const noexcept override { return m_message.c_str(); }
		private:
			std::string m_message;
		};

		/**
		 * @brief Convert a chunk's world position to a relative position in the region
		 * 
		 * @param chunkPos3D 
		 * @param region_pos 
		 * @return glm::ivec2 
		 */
		static glm::ivec2	toRelativePos(const glm::ivec3 & chunkPos3D, const glm::ivec2 & region_pos);

		/**
		 * @brief Get the index ofa chunk in the offset table
		 * 
		 * @param position 
		 * @return size_t 
		 */
		static size_t		getOffsetIndex(const glm::ivec2 & position);

		Region(
			const std::filesystem::path & region_dir,
			const glm::ivec2 & position);

		Region(std::filesystem::path file_path);

		~Region();
		Region(const Region & other) = delete;
		Region(Region && other);
		Region & operator=(const Region & other) = delete;
		Region & operator=(Region && other);

		/**
		 * @brief Get the Position object
		 * 
		 * @return glm::ivec2 
		 */
		glm::ivec2 getPosition() const { return m_position; }

		/**
		 * @brief Get the absolute path to the corresponding region file
		 * 
		 * @return std::filesystem::path 
		 */
		std::filesystem::path getPath() const { return m_path; }

		/**
		 * @brief Tries to return a chunk from the region, might load the region from the disk before.
		 * 
		 * @param chunkPos3D 
		 * @retval std::shared_ptr<Chunk> if the chunk is found
		 * @retval nullptr if the chunk is not found
		 */
		std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & chunkPos3D);

		/**
		 * @brief Add chunk to the list of chunks to save
		 * 
		 * @note you dont have to call this for chunks that you got from getChunk
		 * @param chunk 
		 */
		void					addChunk(const std::shared_ptr<Chunk> & chunk);

		/**
		 * @brief Save the region to the disk, erasing all the chunks from the internal tracking list
		 */
		void					save();

	private:
		std::filesystem::path m_path;
		std::fstream file;
		glm::ivec2 m_position;
		bool m_loaded = false;
		std::unordered_map<glm::ivec3, const std::shared_ptr<Chunk>> m_chunks;
		struct ChunkOffset
		{
			/**
			 * @brief offset of chunk data in the file, in 4096 bytes blocks
			 * 
			 */
			uint32_t offset;

			/**
			 * @brief size of the chunk data in 4096 bytes blocks
			 * 
			 */
			uint32_t size;
		};
		std::unordered_map<glm::ivec2, ChunkOffset> m_offsets;

		void parseOffsets();
		void clearOffsets();
		void writeOffsets();
		void writeChunks();
		void load();
		void readChunk(const glm::ivec2 & relative_position);

		void openFile();
	};
	//first key if the region position
	// if we have a region entry
	//then there is a set to list all the chunks in the save from that region
	//TODO add a way to know if region is full to avoid a list of chunks
	typedef 	std::unordered_map<glm::ivec2, Region> RegionMap;

	RegionMap										m_regions;
	std::unordered_set<std::shared_ptr<Chunk>>		m_chunkPool;


	/****************\
	 * INIT 
	\****************/
	//	open save directory
	//	open a region file
	void initRegions();
};
