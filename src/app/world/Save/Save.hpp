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


	void save(const std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> & chunks);

	void trackChunk(std::shared_ptr<Chunk> chunk);
	
	void saveRegion(const glm::ivec2 & position);

	bool					containsChunk(const glm::ivec3 & position) const;
	std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & position);
private:
	std::filesystem::path m_save_dir;

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

		static glm::ivec2	toRelativePos(const glm::ivec3 & chunkPos3D, const glm::ivec2 & region_pos);
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

		glm::ivec2 getPosition() const { return m_position; }
		std::filesystem::path getPath() const { return m_path; }


		std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & chunkPos3D);
		void					addChunk(const std::shared_ptr<Chunk> & chunk);
		void					save();

	private:
		std::filesystem::path m_path;
		std::fstream file;
		glm::ivec2 m_position;
		bool m_loaded = false;
		std::unordered_map<glm::ivec3, const std::shared_ptr<Chunk>> m_chunks;
		struct ChunkOffset
		{
			uint32_t offset;
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
