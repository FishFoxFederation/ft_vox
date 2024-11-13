#include "Save.hpp"

glm::ivec2 Save::Region::toRegionPos(const glm::ivec3 & chunkPos3D)
{
	return { chunkPos3D.x / REGION_SIZE, chunkPos3D.z / REGION_SIZE };
}

glm::ivec2 Save::Region::toRelativePos(const glm::ivec3 & chunkPos3D, const glm::ivec2 & region_pos)
{
	return {chunkPos3D.x - region_pos.x, chunkPos3D.z - region_pos.y};
}

size_t Save::Region::getOffsetIndex(const glm::ivec2 & position)
{
	return (position.x * REGION_SIZE + position.y);
}

Save::Region::Region(
	const std::filesystem::path & region_dir,
	const glm::ivec2 & position)
:m_position(position)
{

	//create file
	std::string filename = "r."
		+ std::to_string(m_position.x) + "."
		+ std::to_string(m_position.y) + ".ftmc";

	std::filesystem::path file_path = region_dir / filename;

	file.open(file_path, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
	if (!file.good())
	{
		LOG_ERROR("Save: Region: error opening file: " << file_path.string());
		throw std::runtime_error("Save: Region: error opening file: " + file_path.string());
	}
	LOG_INFO("Save: Region: created file: " << file_path.string());
};

Save::Region::Region(std::filesystem::path file_path)
{
	//parse position
	std::string name = file_path.filename().string();
	//name format is r.x.z.ftmc
	//todo check format
	name = name.substr(2); //remove r
	name = name.substr(0, name.find_last_of('.')); //remove extension
	
	//now we have x.z
	std::string x = name.substr(0, name.find('.'));
	std::string z = name.substr(name.find('.') + 1);

	m_position = glm::ivec2(std::stoi(x), std::stoi(z));

	file.open(file_path, std::ios::out | std::ios::in | std::ios::binary);

	if (!file.is_open())
	{
		std::string str = "Save: Region: error opening file: " + file_path.string();
		throw std::runtime_error(str);
	}

	parseOffsets();
}

Save::Region::Region(Region && other)
{
	file = std::move(other.file);
	other.file.close();
	m_position = other.m_position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
}

Save::Region & Save::Region::operator=(Region && other)
{
	file = std::move(other.file);
	other.file.close();
	m_position = other.m_position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
	return *this;
}

Save::Region::~Region()
{
	LOG_INFO("Destroying region: " << m_position.x << " " << m_position.y);
	if (file.is_open())
		save();
}

void Save::Region::save()
{
	LOG_INFO("Saving region: " << m_position.x << " " << m_position.y);
	writeOffsets();
	writeChunks();
}


void Save::Region::writeOffsets()
{
	uint32_t offset = 2; // we start writing chunks after the offset table
	uint32_t size = std::ceil(Chunk::ARRAYS_SIZE / 4096);
	std::array<char, 8192> table_buffer;
	char buffer[8];

	//write offset table
	for (auto & [pos, chunk] : m_chunks)
	{
		glm::ivec2 relative_pos = glm::ivec2(pos.x, pos.z) - m_position;

		m_offsets.insert({relative_pos, {offset, size}});
		std::memcpy(buffer, &offset, 4);
		std::memcpy(buffer + 4, &size, 4);
		offset += size;

		size_t index = getOffsetIndex(relative_pos);		
		memcpy(table_buffer.data() + index, buffer, 8);
	}
	if (!file.is_open())
		throw std::runtime_error("Save: Region: WriteOffsets: file not open");
	file.seekp(0);
	file.write(table_buffer.data(), 1);
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteOffsets: error writing 1");
	// file.seekp(0);
	// if (!file.good())
		// throw std::runtime_error("Save: Region: WriteOffsets: error moving cursor");
	file.write(table_buffer.data(), 8192);
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteOffsets: error writing");
}

void Save::Region::parseOffsets()
{
	// read all 1024 offsets ( 32 * 32 )
	char buffer[8];
	for (size_t i = 0; i < 1024; i++)
	{
		glm::ivec2 pos(i / 32, i % 32);

		file.read(buffer, 8);
		if (!file.good())
			throw std::runtime_error("Save: Region: ParseOffsets: error reading");


		uint32_t * offset = reinterpret_cast<uint32_t *>(buffer);
		uint32_t * size = reinterpret_cast<uint32_t *>(buffer + 4);
		if (*offset != 0)
			m_offsets.insert({pos, { *offset, *size }});
	}	

	for (auto & [pos, offset] : m_offsets)
		LOG_INFO("Offset: " << pos.x << " " << pos.y << " " << offset.offset << " " << offset.size);
}

void Save::Region::writeChunks()
{
	std::array<char, Chunk::ARRAYS_SIZE> buffer;
	file.seekp(2 * 4096); // skip the offset table
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteChunks: error moving cursor");
	for (auto & [pos, chunk] : m_chunks)
	{
		std::lock_guard<Status> lock(chunk->status);
		if (chunk->getGenLevel() != Chunk::genLevel::LIGHT)
			continue;
		glm::ivec2 relative_pos = glm::ivec2(pos.x, pos.z) - m_position;

		uint32_t offset = m_offsets.at(relative_pos).offset;

		Chunk::BlockArray & blocks = chunk->getBlocks();
		Chunk::LightArray & light = chunk->getLight();
		Chunk::BiomeArray & biome = chunk->getBiomes();

		size_t index = 0;
		std::memcpy(buffer.data() + index, blocks.data(), sizeof(Chunk::BlockArray));
		index += sizeof(Chunk::BlockArray);
		std::memcpy(buffer.data() + index, light.data(), sizeof(Chunk::LightArray));
		index += sizeof(Chunk::LightArray);
		std::memcpy(buffer.data() + index, biome.data(), sizeof(Chunk::BiomeArray));

		file.write(buffer.data(), Chunk::ARRAYS_SIZE);
		//padd with zeros
		static std::array<char, Chunk::ARRAYS_SIZE % 4096> zeroes = {};
		file.write(zeroes.data(), Chunk::ARRAYS_SIZE % 4096);
		if (!file.good())
			throw std::runtime_error("Save: Region: WriteChunks: error writing");
	}
}

std::shared_ptr <Chunk> Save::Region::getChunk(const glm::ivec2 & relative_position)
{
	std::array<char, Chunk::ARRAYS_SIZE> buffer;
	auto it = m_offsets.find(relative_position);
	if (it == m_offsets.end())
		return nullptr;
	
	Chunk::BlockArray blocks;
	Chunk::LightArray light;
	Chunk::BiomeArray biome;

	file.seekg(it->second.offset * 4096);
	file.read(buffer.data(), Chunk::ARRAYS_SIZE);

	size_t index = 0;
	std::memcpy(blocks.data(), buffer.data(), sizeof(Chunk::BlockArray));
	index += sizeof(Chunk::BlockArray);
	std::memcpy(light.data(), buffer.data() + index, sizeof(Chunk::LightArray));
	index += sizeof(Chunk::LightArray);
	std::memcpy(biome.data(), buffer.data() + index, sizeof(Chunk::BiomeArray));

	glm::ivec2 pos = relative_position + m_position;
	glm::ivec3 pos3D = {pos.x, 0, pos.y};

	return std::make_shared<Chunk>(pos3D, blocks, light, biome);
}

void Save::Region::addChunk(const std::shared_ptr<Chunk> & chunk)
{
	LOG_INFO("Adding chunk to region: " << m_position.x << " " << m_position.y);
	m_chunks.insert({chunk->getPosition(), chunk});
}
