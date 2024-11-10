#include "ChunkUnloadPacket.hpp"

ChunkUnloadPacket::ChunkUnloadPacket()
: m_chunk_pos(0)
{
}

ChunkUnloadPacket::ChunkUnloadPacket(const glm::ivec3 & chunk_position)
: m_chunk_pos(chunk_position)
{
}

ChunkUnloadPacket::~ChunkUnloadPacket()
{
}

ChunkUnloadPacket::ChunkUnloadPacket(const ChunkUnloadPacket & other)
: IPacket(other), m_chunk_pos(other.m_chunk_pos)
{
}

ChunkUnloadPacket::ChunkUnloadPacket(ChunkUnloadPacket && other)
: IPacket(std::move(other)), m_chunk_pos(other.m_chunk_pos)
{
}

ChunkUnloadPacket & ChunkUnloadPacket::operator=(const ChunkUnloadPacket & other)
{
	if (this != &other)
	{
		m_chunk_pos = other.m_chunk_pos;
		::IPacket::operator=(other);
	}
	return *this;
}

ChunkUnloadPacket & ChunkUnloadPacket::operator=(ChunkUnloadPacket && other)
{
	if (this != &other)
	{
		m_chunk_pos = other.m_chunk_pos;
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

void ChunkUnloadPacket::Serialize(uint8_t * buffer) const
{
	//HEADER
	buffer += SerializeHeader(buffer);

	//BODY
	std::memcpy(buffer, &m_chunk_pos, sizeof(m_chunk_pos));
	buffer += sizeof(m_chunk_pos);
}

void ChunkUnloadPacket::Deserialize(const uint8_t * buffer)
{
	//skip over the packet header
	buffer += IPacket::STATIC_HEADER_SIZE;

	std::memcpy(&m_chunk_pos, buffer, sizeof(m_chunk_pos));
}

uint32_t ChunkUnloadPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_chunk_pos);
}

bool ChunkUnloadPacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type ChunkUnloadPacket::GetType() const
{
	return IPacket::Type::CHUNK_UNLOAD;
}

std::shared_ptr<IPacket> ChunkUnloadPacket::Clone() const
{
	return std::make_shared<ChunkUnloadPacket>();
}

glm::ivec3 ChunkUnloadPacket::GetChunkPosition() const
{
	return m_chunk_pos;
}

void ChunkUnloadPacket::SetChunkPosition(const glm::ivec3 & chunk_position)
{
	m_chunk_pos = chunk_position;
}
