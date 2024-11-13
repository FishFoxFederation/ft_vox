#include "Save.hpp"

Save::Save()
{
}

Save::Save(const std::filesystem::path & path)
:m_save_dir(path)
{
	bool ret = false;

	initRegions();
}


Save::~Save()
{
}

void Save::initRegions()
{
	for(auto & entry : std::filesystem::directory_iterator(m_save_dir))
	{
		if (entry.is_directory())
		{
			std::string str = "Save: initRegion: error directory in region folder:"
				+ entry.path().string();
			throw std::runtime_error(str);
		}

		Region region(entry.path());
		m_regions.emplace(region.getPosition(), std::move(region));
	}
}

void Save::saveChunk(std::shared_ptr<Chunk> chunk)
{
	glm::ivec2 region_pos = Region::toRegionPos(chunk->getPosition());
	glm::ivec2 relative_pos = glm::ivec2(chunk->x(), chunk->z()) - region_pos;

	auto it = m_regions.find(region_pos);
	if (it == m_regions.end())
	{
		Region region(region_pos);
		it = m_regions.emplace(region_pos, std::move(region)).first;
	}

	it->second.addChunk(chunk);
}

void Save::saveRegion(const glm::ivec2 & position)
{
	auto it = m_regions.find(position);
	if (it == m_regions.end())
		return;

	it->second.save();
}
