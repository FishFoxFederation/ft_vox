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
	Save();
	Save(const std::filesystem::path & path);
	~Save();


	void save(const std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> & chunks);
	void saveChunk(std::shared_ptr<Chunk> chunk);
	void saveRegion(const glm::ivec2 & position);

	bool					containsChunk(const glm::ivec3 & position) const;
	std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & position);
private:
	std::filesystem::path m_save_dir;

	class Region
	{
	public:
		static constexpr glm::ivec2 toRegionPos(const glm::ivec3 & chunkPos3D);
		Region(const glm::ivec2 & position);
		Region(std::filesystem::path file_path);
		~Region();
		Region(const Region & other) = delete;
		Region(Region && other);
		Region & operator=(const Region & other) = delete;
		Region & operator=(Region && other);

		glm::ivec2 getPosition() const { return position; }


		static size_t			getOffsetIndex(const glm::ivec2 & position);
		std::shared_ptr<Chunk>	getChunk(const glm::ivec2 & relative_position);
		bool					containsChunk(const glm::ivec2 & relative_position) const;

		void save();
		void addChunk(const std::shared_ptr<Chunk> & chunk);
	private:
		std::basic_fstream<uint8_t> file;
		glm::ivec2 position;
		std::unordered_map<glm::ivec3, const std::shared_ptr<Chunk>> m_chunks;
		struct ChunkOffset
		{
			uint32_t offset;
			uint32_t size;
		};
		std::unordered_map<glm::ivec2, ChunkOffset> m_offsets;
		void parseOffsets();

		void writeOffsets();
		void writeChunks();
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
