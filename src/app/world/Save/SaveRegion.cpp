#include "Save.hpp"

Save::Region::Region(const glm::ivec2 & position)
{
	//create file
};

Save::Region::Region(std::filesystem::path file_path)
{
	//parse position

	std::string name = file_path.filename().string();
	//name format is r.x.z.ftmc
	//todo check format
	name = name.substr(2); //remove r
	name = name.substr(name.find_last_of('.')); //remove extension

	//now we have x.z
	std::string x = name.substr(0, name.find('.'));
	std::string z = name.substr(name.find('.') + 1);

	position = glm::ivec2(std::stoi(x), std::stoi(z));

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
	position = other.position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
}

Save::Region & Save::Region::operator=(Region && other)
{
	file = std::move(other.file);
	position = other.position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
	return *this;
}

Save::Region::~Region()
{
	save();
}

void Save::Region::save()
{
	writeOffsets();
	writeChunks();
}


void Save::Region::writeOffsets()
{
	uint32_t offset = 2; // we start writing chunks after the offset table
	uint32_t size = std::ceil(Chunk::ARRAYS_SIZE / 4096);
	uint8_t buffer[8];

	//write offset table
	for (auto & [pos, chunk] : m_chunks)
	{
		glm::ivec2 relative_pos = glm::ivec2(pos.x, pos.z) - position;

		m_offsets.insert({relative_pos, {offset, size}});
		std::memcpy(buffer, &offset, 4);
		std::memcpy(buffer + 4, &size, 4);
		offset += size;

		size_t index = getOffsetIndex(relative_pos);		
		file.seekp(index);
		file.write(buffer, 8);
	}
}

void Save::Region::parseOffsets()
{
	// read all 1024 offsets ( 32 * 32 )
	uint8_t buffer[8];
	for (size_t i = 0; i < 1024; i++)
	{
		glm::ivec2 pos(i / 32, i % 32);

		file.read(buffer, 8);

		//todo check read error

		uint32_t * offset = reinterpret_cast<uint32_t *>(buffer);
		uint32_t * size = reinterpret_cast<uint32_t *>(buffer + 4);
		if (*offset != 0)
			m_offsets.insert({pos, { *offset, *size }});
	}	
}

void Save::Region::writeChunks()
{
	uint8_t buffer[Chunk::ARRAYS_SIZE];
	for (auto & [pos, chunk] : m_chunks)
	{
		std::lock_guard<Status> lock(chunk->status);
		glm::ivec2 relative_pos = glm::ivec2(pos.x, pos.z) - position;

		uint32_t offset = m_offsets.at(relative_pos).offset;

		Chunk::BlockArray & blocks = chunk->getBlocks();
		Chunk::LightArray & light = chunk->getLight();
		Chunk::BiomeArray & biome = chunk->getBiomes();
		Chunk::HeightArray & height = chunk->getHeights();

		size_t index = 0;
		std::memcpy(buffer + index, blocks.data(), sizeof(Chunk::BlockArray));
		index += sizeof(Chunk::BlockArray);
		std::memcpy(buffer + index, light.data(), sizeof(Chunk::LightArray));
		index += sizeof(Chunk::LightArray);
		std::memcpy(buffer + index, biome.data(), sizeof(Chunk::BiomeArray));
		index += sizeof(Chunk::BiomeArray);
		std::memcpy(buffer + index, height.data(), sizeof(Chunk::HeightArray));

		file.seekp(offset * 4096);
		file.write(buffer, Chunk::ARRAYS_SIZE);
	}
}

std::shared_ptr <Chunk> Save::Region::getChunk(const glm::ivec2 & relative_position)
{
	uint8_t buffer[Chunk::ARRAYS_SIZE];
	auto it = m_offsets.find(relative_position);
	if (it == m_offsets.end())
		return nullptr;
	
	Chunk::BlockArray blocks;
	Chunk::LightArray light;
	Chunk::BiomeArray biome;
	Chunk::HeightArray height;

	file.seekg(it->second.offset);
	file.read(buffer, Chunk::ARRAYS_SIZE);

	size_t index = 0;
	std::memcpy(blocks.data(), buffer, sizeof(Chunk::BlockArray));
	index += sizeof(Chunk::BlockArray);
	std::memcpy(light.data(), buffer + index, sizeof(Chunk::LightArray));
	index += sizeof(Chunk::LightArray);
	std::memcpy(biome.data(), buffer + index, sizeof(Chunk::BiomeArray));
	index += sizeof(Chunk::BiomeArray);
	std::memcpy(height.data(), buffer + index, sizeof(Chunk::HeightArray));

	glm::ivec2 pos = relative_position + position;
	glm::ivec3 pos3D = {pos.x, 0, pos.y};

	return std::make_shared<Chunk>(pos3D, blocks, light, biome, height);
}
