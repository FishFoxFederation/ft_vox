#include "ChunkPacket.hpp"

ChunkPacket::ChunkPacket()
: m_chunk_pos(0), m_blocks()
{
}

ChunkPacket::ChunkPacket(const Chunk & chunk)
: m_chunk_pos(chunk.getPosition()), m_blocks(chunk.getBlocks())
{
}

ChunkPacket::~ChunkPacket()
{
}

ChunkPacket::ChunkPacket(ChunkPacket & other)
: IPacket(other), m_chunk_pos(other.m_chunk_pos), m_blocks(other.m_blocks)
{
}

ChunkPacket::ChunkPacket(ChunkPacket && other)
: IPacket(std::move(other)), m_chunk_pos(other.m_chunk_pos), m_blocks(other.m_blocks)
{
}

ChunkPacket & ChunkPacket::operator=(ChunkPacket & other)
{
	if (this != &other)
	{
		m_chunk_pos = other.m_chunk_pos;
		m_blocks = other.m_blocks;
		::IPacket::operator=(other);
	}
	return *this;
}

ChunkPacket & ChunkPacket::operator=(ChunkPacket && other)
{
	if (this != &other)
	{
		m_chunk_pos = other.m_chunk_pos;
		m_blocks = std::move(other.m_blocks);
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

void ChunkPacket::Serialize(uint8_t * buffer) const
{
	auto type = GetType();
	std::memcpy(buffer, &type, sizeof(type));
	buffer += sizeof(type);

	std::memcpy(buffer, &m_chunk_pos, sizeof(m_chunk_pos));
	buffer += sizeof(m_chunk_pos);

	std::memcpy(buffer, &m_blocks, sizeof(m_blocks));
	buffer += sizeof(m_blocks);
}

void ChunkPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(IPacket::STATIC_HEADER_SIZE);

}

uint32_t ChunkPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_chunk_pos) + sizeof(m_blocks);
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

Chunk	ChunkPacket::GetChunk() const
{
	return Chunk(m_chunk_pos, m_blocks);
}

void ChunkPacket::SetChunk(const Chunk & chunk)
{
	m_chunk_pos = chunk.getPosition();
	m_blocks = chunk.getBlocks();
}
