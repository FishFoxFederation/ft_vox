#pragma once

#include <filesystem>
#include <fstream>
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

	bool					containsChunk(const glm::ivec3 & position) const;
	std::shared_ptr<Chunk>	getChunk(const glm::ivec3 & position);
	// void saveChunk(std::shared_ptr<Chunk> chunk);
private:
	std::filesystem::path  m_path;

	struct Region
	{
		Region(std::filesystem::path save_dir, glm::ivec2 region_pos);
		~Region();
		std::vector<std::shared_ptr<Chunk>> getChunks();

		std::ifstream file;
		std::unordered_set<glm::ivec3> chunks;
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
};
