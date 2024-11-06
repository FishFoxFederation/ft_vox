#include "ChunkPacket.hpp"

ChunkPacket::ChunkPacket()
: m_chunk_data()
{
}

ChunkPacket::ChunkPacket(const Chunk & chunk)
: m_chunk_data(chunk.getPosition(), chunk.getBlocks(), chunk.getLight(), chunk.getBiomes())
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
	ZoneScoped;
	auto type = GetType();
	std::memcpy(buffer, &type, sizeof(type));
	buffer += sizeof(type);

	std::memcpy(buffer, &m_chunk_data, sizeof(m_chunk_data));
	buffer += sizeof(m_chunk_data);
}

void ChunkPacket::Deserialize(const uint8_t * buffer)
{
	ZoneScoped;
	buffer += sizeof(IPacket::STATIC_HEADER_SIZE);

	std::memcpy(&m_chunk_data, buffer, sizeof(m_chunk_data));
	buffer += sizeof(m_chunk_data);
}

uint32_t ChunkPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_chunk_data);
}

bool ChunkPacket::HasDynamicSize() const
{
	return false;
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
	ZoneScoped;
	return std::make_shared<Chunk>(m_chunk_data.chunk_pos, m_chunk_data.blocks, m_chunk_data.light, m_chunk_data.biomes);
}

void ChunkPacket::SetChunk(const Chunk & chunk)
{
	ZoneScoped;
	m_chunk_data = ChunkData(chunk.getPosition(), chunk.getBlocks(), chunk.getLight(), chunk.getBiomes());
}
