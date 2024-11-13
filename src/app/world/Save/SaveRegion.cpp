#include "Save.hpp"
#include "RLE_TEST.hpp"

glm::ivec2 Save::Region::toRegionPos( glm::ivec3 chunkPos3D)
{
	glm::ivec2 ret;

	if (chunkPos3D.x < 0)
		chunkPos3D.x -= REGION_SIZE - 1;
	if (chunkPos3D.z < 0)
		chunkPos3D.z -= REGION_SIZE - 1;
	ret.x = chunkPos3D.x - (chunkPos3D.x % REGION_SIZE);
	ret.y = chunkPos3D.z - (chunkPos3D.z % REGION_SIZE);
	return ret;
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

	file.open(file_path, std::ios::in | std::ios::out | std::ios::binary);

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
	if (file.is_open())
		save();
}

void Save::Region::save()
{
	LOG_INFO("Saving region: " << m_position.x << " " << m_position.y);
	clearOffsets();
	writeChunks();
	writeOffsets();
}

void Save::Region::clearOffsets()
{
	file.seekp(0);
	file.write(std::string(8192, '\0').c_str(), 8192);
}

void Save::Region::writeOffsets()
{
	std::array<char, 8192> table_buffer = {};
	char buffer[8];

	//write offset table
	for (auto & [pos, info] : m_offsets)
	{
		uint32_t offset = info.offset;
		uint32_t size = info.size;
		std::memcpy(buffer, &offset, 4);
		std::memcpy(buffer + 4, &size, 4);
		size_t index = getOffsetIndex(pos) * 8;
		std::memcpy(table_buffer.data() + index, buffer, 8);
		LOG_INFO("Offset: " << pos.x << " " << pos.y << " " << offset << " " << size);
	}
	if (!file.is_open())
		throw std::runtime_error("Save: Region: WriteOffsets: file not open");
	file.seekp(0);
	// file.write(table_buffer.data(), 1);
	// if (!file.good())
		// throw std::runtime_error("Save: Region: WriteOffsets: error writing 1");
	// file.seekp(0);
	// if (!file.good())
		// throw std::runtime_error("Save: Region: WriteOffsets: error moving cursor");
	file.write(table_buffer.data(), sizeof(table_buffer));
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteOffsets: error writing");
}

void Save::Region::parseOffsets()
{
	LOG_INFO("Parsing offsets " << m_position.x << " " << m_position.y);
	// read all 1024 offsets ( 32 * 32 )
	char buffer[8];
	for (size_t i = 0; i < 1024; i++)
	{
		glm::ivec2 pos(i / 32, i % 32);

		file.read(buffer, 8);
		if (!file.good())
			throw std::runtime_error("Save: Region: ParseOffsets: error reading");


		uint32_t offset = 0;
		std::memcpy(&offset, buffer, 4);
		uint32_t size = 0;
		std::memcpy(&size, buffer + 4, 4);
		if (offset != 0)
			m_offsets.insert({pos, { offset, size }});
	}	

	for (auto & [pos, offset] : m_offsets)
		LOG_INFO("Offset: " << pos.x << " " << pos.y << " " << offset.offset << " " << offset.size);
}

void Save::Region::writeChunks()
{
	std::vector<char> buffer;
	file.seekp(2 * 4096); // skip the offset table

	uint32_t offset = 2;
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteChunks: error moving cursor");
	for (auto & [pos, chunk] : m_chunks)
	{
		LOG_INFO("Writing CHUNK " << pos.x << " " << pos.z);
		std::lock_guard<Status> lock(chunk->status);
		glm::ivec2 relative_pos = toRelativePos(pos, m_position);

		RLE_TEST blocks(chunk->getBlocks());
		RLE_TEST light(chunk->getLight());
		RLE_TEST biome(chunk->getBiomes());

		Chunk::genLevel genLevel = chunk->getGenLevel();

		size_t total_size = blocks.getRawSize() + light.getRawSize() + biome.getRawSize() + sizeof(Chunk::genLevel) + 3 * sizeof(size_t);
		size_t index = 0;

		buffer.resize(total_size);

		size_t size = blocks.getRawSize();
		std::memcpy(buffer.data() + index, &size, sizeof(size_t));
		index += sizeof(size_t);
		std::memcpy(buffer.data() + index, blocks.getRaw().data(), size);
		index += size;

		size = light.getRawSize();
		std::memcpy(buffer.data() + index, &size, sizeof(size_t));
		index += sizeof(size_t);
		std::memcpy(buffer.data() + index, light.getRaw().data(), size);
		index += size;

		size = biome.getRawSize();
		std::memcpy(buffer.data() + index, &size, sizeof(size_t));
		index += sizeof(size_t);
		std::memcpy(buffer.data() + index, biome.getRaw().data(), size);
		index += size;

		size = sizeof(Chunk::genLevel);
		std::memcpy(buffer.data() + index, &genLevel, size);
		index += size;

		//calculate padding
		size_t padding = 4096 - total_size % 4096;
		if (padding != 4096)
			buffer.insert(buffer.end(), padding, '\0');

		file.write(buffer.data(), buffer.size());

		uint32_t zone_size = buffer.size() / 4096 + (buffer.size() % 4096 != 0);
		m_offsets[relative_pos] = {offset, zone_size};
		offset += zone_size;
		if (!file.good())
			throw std::runtime_error("Save: Region: WriteChunks: error writing");
		auto cursor_pos = file.tellp();
	}
}

std::shared_ptr <Chunk> Save::Region::getChunk(const glm::ivec3 & chunkPos3D)
{
	if (!m_has_read)
		readChunks();
	auto it = m_chunks.find(chunkPos3D);
	if (it == m_chunks.end())
		return nullptr;
	return it->second;
}

void Save::Region::addChunk(const std::shared_ptr<Chunk> & chunk)
{
	LOG_INFO("Adding chunk to region: " << m_position.x << " " << m_position.y);
	m_chunks.insert({chunk->getPosition(), chunk});
}


void Save::Region::readChunks()
{
	for(auto & [pos, offset] : m_offsets)
		readChunk(pos);
	m_has_read = true;
}

void Save::Region::readChunk(const glm::ivec2 & relative_position)
{
	auto it = m_offsets.find(relative_position);
	if (it == m_offsets.end())
	{
		LOG_INFO("Chunk not found: " << relative_position.x << " " << relative_position.y);
		return;
	}
	std::vector<char> buffer(it->second.size * 4096);

	RLE_TEST blocks = Chunk::BlockArray();
	RLE_TEST light = Chunk::LightArray();
	RLE_TEST biome  = Chunk::BiomeArray();
	Chunk::genLevel genLevel;

	file.seekg(it->second.offset * 4096);
	file.read(buffer.data(), buffer.size());

	size_t index = 0;
	size_t size = 0;


	std::memcpy(&size, buffer.data(), sizeof(size_t));
	index += sizeof(size_t);
	blocks.resize(size);
	std::memcpy(blocks.getRaw().data(), buffer.data() + index, size);
	index += size;

	std::memcpy(&size, buffer.data(), sizeof(size_t));
	index += sizeof(size_t);
	light.resize(size);
	std::memcpy(light.getRaw().data(), buffer.data() + index, size);
	index += size;

	std::memcpy(&size, buffer.data(), sizeof(size_t));
	index += sizeof(size_t);
	biome.resize(size);
	std::memcpy(biome.getRaw().data(), buffer.data() + index, size);
	index += size;

	std::memcpy(&genLevel, buffer.data() + index, sizeof(Chunk::genLevel));

	glm::ivec2 pos = relative_position + m_position;
	glm::ivec3 pos3D = {pos.x, 0, pos.y};
	auto chunk = std::make_shared<Chunk>(pos3D, blocks.getData(), light.getData(), biome.getData());
	chunk->setGenLevel(genLevel);
	m_chunks.insert({pos3D, chunk});
}
