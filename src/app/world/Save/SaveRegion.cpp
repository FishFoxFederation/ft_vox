#include "Save.hpp"
#include "RLE_TEST.hpp"

glm::ivec2 Save::toRegionPos( glm::ivec3 chunkPos3D)
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

	//create filename
	std::string filename = "r."
		+ std::to_string(m_position.x) + "."
		+ std::to_string(m_position.y) + ".ftmc";

	m_path = region_dir / filename;

	//create file
	file.open(m_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		throw std::runtime_error("Save: Region: error creating file");
	file.close();
};

Save::Region::Region(std::filesystem::path file_path)
: m_path(file_path)
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

	openFile();
	parseOffsets();
	file.close();
}

Save::Region::Region(Region && other)
{
	file = std::move(other.file);
	m_path = std::move(other.m_path);
	m_position = other.m_position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
	other.m_loaded = false;
}

Save::Region & Save::Region::operator=(Region && other)
{
	file = std::move(other.file);
	m_path = std::move(other.m_path);
	m_position = other.m_position;
	m_chunks = std::move(other.m_chunks);
	m_offsets = std::move(other.m_offsets);
	other.m_loaded = false;
	return *this;
}

Save::Region::~Region()
{
	if (m_loaded == true)
		save();
}

void Save::Region::save()
{
	openFile();
	clearOffsets();
	writeChunks();
	writeOffsets();
	
	m_chunks.clear();
	m_loaded = false;
	file.close();
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
		// LOG_INFO("Offset: " << pos.x << " " << pos.y << " " << offset << " " << size);
	}
	file.seekp(0);
	file.write(table_buffer.data(), sizeof(table_buffer));
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteOffsets: error writing");
}

void Save::Region::parseOffsets()
{
	// read all 1024 offsets ( 32 * 32 )
	char buffer[8];
	file.seekg(0);

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
}

void Save::Region::writeChunks()
{
	std::vector<char> buffer;
	file.seekp(2 * 4096); // skip the offset table
	// LOG_INFO("Save: Region: WriteChunks: " << m_position.x << " " << m_position.y);
	uint32_t offset = 2;
	if (!file.good())
		throw std::runtime_error("Save: Region: WriteChunks: error moving cursor");
	for (auto & [pos, chunk] : m_chunks)
	{
		std::lock_guard<Status> lock(chunk->status);
		glm::ivec2 relative_pos = toRelativePos(pos, m_position);

		ChunkData data(*chunk);

		buffer = std::move(data.serialize());
		size_t write_size = buffer.size();

		file.write(reinterpret_cast<const char *>(&write_size), sizeof(size_t));
		write_size += sizeof(size_t);

		//calculate padding
		size_t padding = 4096 - write_size % 4096;
		// if (padding != 4096)
		buffer.insert(buffer.end(), padding, '\0');

		file.write(buffer.data(), buffer.size());
		if ((write_size + padding) % 4096 != 0)
			throw std::runtime_error("Save: Region: WriteChunks: error padding");
		// LOG_INFO("Save: Region: WriteChunks: " << pos.x << " " << pos.z << " " << write_size);
		uint32_t zone_size = (write_size + padding) / 4096;
		m_offsets[relative_pos] = {offset, zone_size};
		offset += zone_size;
		if (!file.good())
			throw std::runtime_error("Save: Region: WriteChunks: error writing");
		auto cursor_pos = file.tellp();
	}
}

std::shared_ptr <Chunk> Save::Region::getChunk(const glm::ivec3 & chunkPos3D)
{
	if (!m_loaded)
		load();
	auto it = m_chunks.find(chunkPos3D);
	if (it == m_chunks.end())
		return nullptr;
	return it->second;
}

void Save::Region::addChunk(const std::shared_ptr<Chunk> & chunk)
{
	m_chunks.insert({chunk->getPosition(), chunk});
}

void Save::Region::load()
{
	openFile();
	for(auto & [pos, offset] : m_offsets)
		readChunk(pos);
	m_loaded = true;
	file.close();
}

void Save::Region::readChunk(const glm::ivec2 & relative_position)
{
	auto it = m_offsets.find(relative_position);
	if (it == m_offsets.end())
		return;

	glm::ivec3 expected_pos = {relative_position.x + m_position.x, 0, relative_position.y + m_position.y};
	std::vector<char> buffer(it->second.size * 4096);
	ChunkData data;
	size_t size = 0;

	//read all data from file
	file.seekg(it->second.offset * 4096);
	file.read(buffer.data(), buffer.size());
	if (!file.good())
		throw std::runtime_error("Save: Region: ReadChunk: error reading");

	//extract size then deserialize data
	std::memcpy(&size, buffer.data(), sizeof(size_t));
	// LOG_INFO("Save: Region: ReadChunk: " << expected_pos.x << " " << expected_pos.z << " " << size);
	data.deserialize(buffer.data() + sizeof(size_t), size);

	auto chunk = std::make_shared<Chunk>(data);

	if (chunk->getPosition() != expected_pos)
	{
		LOG_ERROR("Save: Region: ReadChunk: error reading chunk: expected position: " << expected_pos.x << " " << expected_pos.z << " got: " << chunk->getPosition().x << " " << chunk->getPosition().z);
		return;
	}
	//store chunk to keep track of it
	m_chunks.insert({chunk->getPosition(), chunk});
}

void Save::Region::openFile()
{
	if (file.is_open())
		return;
	file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);

	if (!file.is_open())
	{
		std::string str = "Save: Region: error opening file: " + m_path.string();
		throw std::runtime_error(str);
	}
}
