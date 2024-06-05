#include "ChunkRequestPacket.hpp"

ChunkRequestPacket::ChunkRequestPacket()
{
}

ChunkRequestPacket::~ChunkRequestPacket()
{
}

ChunkRequestPacket::ChunkRequestPacket(ChunkRequestPacket & other)
{
	m_chunk_pos = other.m_chunk_pos;
}

ChunkRequestPacket & ChunkRequestPacket::operator=(ChunkRequestPacket & other)
{
	m_chunk_pos = other.m_chunk_pos;
	return *this;
}

ChunkRequestPacket::ChunkRequestPacket(ChunkRequestPacket && other)
{
	m_chunk_pos = other.m_chunk_pos;
}

ChunkRequestPacket & ChunkRequestPacket::operator=(ChunkRequestPacket && other)
{
	m_chunk_pos = other.m_chunk_pos;
	return *this;
}

void ChunkRequestPacket::Serialize(uint8_t * buffer) const
{
	auto type = GetType();
	memcpy(buffer, &type, sizeof(IPacket::Type));
	buffer += sizeof(IPacket::Type);

	memcpy(buffer, &m_chunk_pos, sizeof(glm::ivec3));
}

void ChunkRequestPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(IPacket::STATIC_HEADER_SIZE);
	
	memcpy(&m_chunk_pos, buffer, sizeof(glm::ivec3));
}

uint32_t ChunkRequestPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(glm::ivec3);
}

bool ChunkRequestPacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type ChunkRequestPacket::GetType() const
{
	return IPacket::Type::CHUNK_REQUEST;
}

std::shared_ptr<IPacket> ChunkRequestPacket::Clone() const
{
	return std::make_shared<ChunkRequestPacket>();
}

glm::ivec3 ChunkRequestPacket::GetChunkPos() const
{
	return m_chunk_pos;
}

void ChunkRequestPacket::SetChunkPos(const glm::ivec3 & chunk_pos)
{
	m_chunk_pos = chunk_pos;
}
