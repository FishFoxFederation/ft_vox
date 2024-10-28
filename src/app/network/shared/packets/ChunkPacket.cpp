#include "ChunkPacket.hpp"

ChunkPacket::ChunkPacket()
: m_chunk_data()
{
}

ChunkPacket::ChunkPacket(const Chunk & chunk)
: m_chunk_data(chunk.getPosition(), chunk.getLight(), chunk.getBiomes()), m_blocks(chunk.getBlocks())
{
}

ChunkPacket::~ChunkPacket()
{
}

ChunkPacket::ChunkPacket(ChunkPacket & other)
: IPacket(other), m_chunk_data(other.m_chunk_data)
{
}

ChunkPacket::ChunkPacket(ChunkPacket && other)
: IPacket(std::move(other)), m_chunk_data(other.m_chunk_data)
{
}

ChunkPacket & ChunkPacket::operator=(ChunkPacket & other)
{
	if (this != &other)
	{
		m_chunk_data = other.m_chunk_data;
		::IPacket::operator=(other);
	}
	return *this;
}

ChunkPacket & ChunkPacket::operator=(ChunkPacket && other)
{
	if (this != &other)
	{
		m_chunk_data = std::move(other.m_chunk_data);
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

void ChunkPacket::Serialize(uint8_t * buffer) const
{
	auto type = GetType();
	std::memcpy(buffer, &type, sizeof(type));
	buffer += sizeof(type);

	//encode size
	size_t size = Size();
	std::memcpy(buffer, &size, sizeof(size));
	buffer += sizeof(size);

	std::memcpy(buffer, &m_chunk_data, sizeof(m_chunk_data));
	buffer += sizeof(m_chunk_data);

	std::memcpy(buffer, m_blocks.getRaw().data(), m_blocks.getRawSize());
	buffer += m_blocks.getRawSize();
}

void ChunkPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(IPacket::Type);

	size_t size_of_blocks;
	std::memcpy(&size_of_blocks, buffer, sizeof(size_of_blocks));
	buffer += sizeof(size_of_blocks);

	size_of_blocks -= IPacket::DYNAMIC_HEADER_SIZE;
	size_of_blocks -= sizeof(m_chunk_data);

	std::memcpy(&m_chunk_data, buffer, sizeof(m_chunk_data));
	buffer += sizeof(m_chunk_data);

	m_blocks.getRaw().resize(size_of_blocks / sizeof(m_blocks.sizeOfPair()));

	std::memcpy(m_blocks.getRaw().data(), buffer, size_of_blocks);
	buffer += size_of_blocks;
}

uint32_t ChunkPacket::Size() const
{
	return IPacket::DYNAMIC_HEADER_SIZE + sizeof(m_chunk_data) + m_blocks.getRawSize();
}

bool ChunkPacket::HasDynamicSize() const
{
	return true;
}

IPacket::Type ChunkPacket::GetType() const
{
	return IPacket::Type::CHUNK;
}

std::shared_ptr<IPacket> ChunkPacket::Clone() const
{
	return std::make_shared<ChunkPacket>();
}

std::shared_ptr<Chunk>	ChunkPacket::GetChunk() const
{
	return std::make_shared<Chunk>(m_chunk_data.chunk_pos, m_blocks.getData(), m_chunk_data.light, m_chunk_data.biomes);
}

void ChunkPacket::SetChunk(const Chunk & chunk)
{
	m_chunk_data = ChunkData(chunk.getPosition(), chunk.getLight(), chunk.getBiomes());
	m_blocks.setData(chunk.getBlocks());
}
