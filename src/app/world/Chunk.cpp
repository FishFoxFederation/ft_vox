#include "Chunk.hpp"
#include "logger.hpp"
#include <cstring>

Chunk::Chunk(glm::ivec3 position)
: position(position), m_mesh_id(0)
{
	(void)position;
	// LOG_INFO("Chunk created at position: " << position.x << " " << position.y << " " << position.z);
	for(int i = 0; i < BLOCKS_PER_CHUNK; i++)
		m_blocks[i] = BlockID::Air;

	memset(m_light.data(), 0, BLOCKS_PER_CHUNK);
}

// Chunk::Chunk(const glm::ivec3 & position, const BlockArray & blocks)
// : position(position), m_mesh_id(0), m_blocks(blocks)
// {
// 	memset(m_light.data(), 0, BLOCKS_PER_CHUNK);
// }

Chunk::Chunk(const glm::ivec3 & position, const BlockArray & blocks, const LightArray & light)
: position(position), m_mesh_id(0), m_blocks(blocks), m_light(light)
{
}

// Chunk::Chunk(const Chunk & other)
// :	position(other.position),
// 	m_mesh_id(other.m_mesh_id),
// 	m_blocks(other.m_blocks),
// 	m_light(other.m_light)
// {
// }

// Chunk::Chunk(Chunk && other)
// :
// 	// status(other.status),
// 	position(other.position),
// 	m_mesh_id(other.m_mesh_id),
// 	m_blocks(std::move(other.m_blocks)),
// 	m_light(std::move(other.m_light))
// {
// }

// Chunk & Chunk::operator=(const Chunk & other)
// {
// 	position = other.position;
// 	m_mesh_id = other.m_mesh_id;
// 	m_blocks = other.m_blocks;
// 	m_light = other.m_light;
// 	// status = other.status;
// 	return *this;
// }

// Chunk & Chunk::operator=(const Chunk && other)
// {
// 	position = other.position;
// 	m_mesh_id = other.m_mesh_id;
// 	m_blocks = std::move(other.m_blocks);
// 	m_light = std::move(other.m_light);
// 	// status = other.status;
// 	return *this;
// }

Chunk::~Chunk()
{
	status.try_lock();
}

Chunk::BlockArray & Chunk::getBlocks()
{
	return m_blocks;
}

const Chunk::BlockArray & Chunk::getBlocks() const
{
	return m_blocks;
}

BlockID Chunk::getBlock(const int & x, const int & y, const int & z) const
{
	int index = toIndex(x, y, z);

	return m_blocks[index];
}

BlockID Chunk::getBlock(const glm::ivec3 & position) const
{
	return getBlock(position.x, position.y, position.z);
}

void Chunk::setBlock(const int & x, const int & y, const int & z, BlockID block)
{
	int index = toIndex(x, y, z);

	m_blocks[index] = block;
	//REGENERATE MESH HERE
}

void Chunk::setBlock(const glm::ivec3 & position, BlockID block)
{
	setBlock(position.x, position.y, position.z, block);
}

Chunk::LightArray & Chunk::getLight()
{
	return m_light;
}

const Chunk::LightArray & Chunk::getLight() const
{
	return m_light;
}

uint8_t Chunk::getLight(const int & x, const int & y, const int & z) const
{
	int index = toIndex(x, y, z);

	return m_light[index];
}

uint8_t Chunk::getLight(const glm::ivec3 & position) const
{
	return getLight(position.x, position.y, position.z);
}

void Chunk::setLight(const int & x, const int & y, const int & z, uint8_t light)
{
	int index = toIndex(x, y, z);

	m_light[index] = light;
}

void Chunk::setLight(const glm::ivec3 & position, uint8_t light)
{
	setLight(position.x, position.y, position.z, light);
}

const glm::ivec3 & Chunk::getPosition() const
{
	return position;
}

void Chunk::setPosition(const glm::ivec3 & position)
{
	this->position = position;
}

const uint64_t & Chunk::getMeshID() const
{
	return m_mesh_id;
}

void Chunk::setMeshID(const uint64_t & mesh_id)
{
	m_mesh_id = mesh_id;
}

bool Chunk::isMeshed() const
{
	return meshed;
}

void Chunk::setMeshed(bool meshed)
{
	this->meshed = meshed;
}

int Chunk::getLoadLevel() const
{
	return load_level;
}

int Chunk::getHighestLoadLevel() const
{
	return highest_load_level;
}
void Chunk::setLoadLevel(const int & load_level)
{
	this->load_level = load_level;
	if (this->load_level > highest_load_level)
		highest_load_level = this->load_level;
}

Chunk::genLevel Chunk::getGenLevel() const
{
	return m_gen_level;
}

void Chunk::setGenLevel(const genLevel & level)
{
	m_gen_level = level;
}

int Chunk::toIndex(const int & x, const int & y, const int & z)
{
	return x + y * CHUNK_X_SIZE + z * CHUNK_X_SIZE * CHUNK_Y_SIZE;
}

glm::ivec3	Chunk::toCoord(const int & index)
{
	int x = index % CHUNK_X_SIZE;
	int y = (index - x) % CHUNK_Y_SIZE;
	int z = (index - x - y) % CHUNK_Z_SIZE;
	return glm::ivec3(x, y, z);
}
