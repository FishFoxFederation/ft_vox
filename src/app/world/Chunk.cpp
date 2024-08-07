#include "Chunk.hpp"
#include "logger.hpp"

Chunk::Chunk(glm::ivec3 position)
: position(position), m_mesh_id(0)
{
	(void)position;
	// LOG_INFO("Chunk created at position: " << position.x << " " << position.y << " " << position.z);
	for(int i = 0; i < BLOCKS_PER_CHUNK; i++)
		m_blocks[i] = BlockID::Air;
}

Chunk::Chunk(const glm::ivec3 & position, const BlockArray & blocks)
: position(position), m_mesh_id(0), m_blocks(blocks)
{
}

Chunk::Chunk(const Chunk & other)
:	position(other.position),
	m_mesh_id(other.m_mesh_id),
	m_blocks(other.m_blocks)
{
}

Chunk::Chunk(Chunk && other)
:	
	// status(other.status),
	position(other.position),
	m_mesh_id(other.m_mesh_id),
	m_blocks(std::move(other.m_blocks))
{
}

Chunk & Chunk::operator=(const Chunk & other)
{
	position = other.position;
	m_mesh_id = other.m_mesh_id;
	m_blocks = other.m_blocks;
	// status = other.status;
	return *this;
}

Chunk & Chunk::operator=(const Chunk && other)
{
	position = other.position;
	m_mesh_id = other.m_mesh_id;
	m_blocks = std::move(other.m_blocks);
	// status = other.status;
	return *this;
}

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

bool Chunk::isGenerated() const
{
	return gen;
}

void Chunk::setGenerated(bool gen)
{
	this->gen = gen;
}

int Chunk::getLoadLevel() const
{
	return load_level;
}

void Chunk::setLoadLevel(const int & load_level)
{
	this->load_level = load_level;
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
