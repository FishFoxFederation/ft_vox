#include "Save.hpp"

const std::filesystem::path Save::DEFAULT_NAME{ "myWorld" };
const std::filesystem::path Save::SAVE_DIR{ "saves" };

Save::Save()
: m_save_dir(SAVE_DIR / DEFAULT_NAME)
{
	bool ret = true;
	if (std::filesystem::exists(m_save_dir))
		initRegions();
	else
		ret = std::filesystem::create_directories(m_save_dir);
	if (ret == false)
		throw std::runtime_error("Save: Save: error creating save directory: " + m_save_dir.string());
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

		try {
			Region region(entry.path());
			m_regions.emplace(region.getPosition(), std::move(region));
		} catch (Region::CorruptedFileException & e) {
			LOG_ERROR("Save: initRegions: " << e.what());
			std::filesystem::remove(entry.path());
		}
	}
}

void Save::trackChunk(std::shared_ptr<Chunk> chunk)
{
	glm::ivec2 region_pos = toRegionPos(chunk->getPosition());

	//find corresponding region if not found create it
	auto it = m_regions.find(region_pos);
	if (it == m_regions.end())
	{
		Region region(m_save_dir, region_pos);
		it = m_regions.emplace(region_pos, std::move(region)).first;
	}

	//add chunk to the region's tracked chunks
	it->second.addChunk(chunk);
}

void Save::saveRegion(const glm::ivec2 & position)
{
	auto it = m_regions.find(position);
	if (it == m_regions.end())
		return;

	it->second.save();
}

std::shared_ptr<Chunk> Save::getChunk(const glm::ivec3 & position)
{
	glm::ivec2 region_pos = toRegionPos(position);

	auto it = m_regions.find(region_pos);
	if (it == m_regions.end())
		return nullptr;
	std::shared_ptr<Chunk> ret;

	try {
		ret = it->second.getChunk(position);
	} catch (Region::CorruptedFileException & e) {
		LOG_ERROR("Save: getChunk: " << e.what());
		std::filesystem::remove(m_save_dir / ("r." + std::to_string(region_pos.x) + "." + std::to_string(region_pos.y) + ".ftmc"));
		m_regions.erase(it);

		Region region(m_save_dir, region_pos);
		m_regions.insert({region_pos, std::move(region)});
		return nullptr;
	}

	return it->second.getChunk(position);
}
