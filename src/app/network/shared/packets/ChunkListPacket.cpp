#include "ChunkListPacket.hpp"

ChunkListPacket::ChunkListPacket()
: m_chunks()
{
}

ChunkListPacket::ChunkListPacket(const std::vector<std::shared_ptr<Chunk>> & chunks)
{
	for (auto & chunk : chunks)
		m_chunks.push_back(std::make_pair(ChunkPacket::ChunkData(chunk->getPosition(), chunk->getLight(), chunk->getBiomes()), BlockRLE(chunk->getBlocks())));
}

ChunkPacket::~ChunkPacket()
{
}

ChunkListPacket::ChunkListPacket(const ChunkListPacket& other)
: IPacket(other), m_chunks(other.m_chunks)
{
}

ChunkListPacket::ChunkListPacket(ChunkListPacket&& other)
: IPacket(std::move(other)), m_chunks(std::move(other.m_chunks))
{
}

ChunkListPacket& ChunkListPacket::operator=(const ChunkListPacket& other)
{
	if (this != &other)
	{
		m_chunks = other.m_chunks;
		::IPacket::operator=(other);
	}
	return *this;
}

ChunkListPacket& ChunkListPacket::operator=(ChunkListPacket&& other)
{
	if (this != &other)
	{
		m_chunks = std::move(other.m_chunks);
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

void ChunkListPacket::Serialize(uint8_t * buffer) const
{
	auto type = GetType();
	std::memcpy(buffer, &type, sizeof(type));
	buffer += sizeof(type);

	//encode size
	size_t size = Size();
	std::memcpy(buffer, &size, sizeof(size));
	buffer += sizeof(size);

	for (auto  [data, block ] : m_chunks)
	{
		std::memcpy(buffer, &data, sizeof(data));
		buffer += sizeof(data);
		std::memcpy(buffer, block.getRaw().data(), block.getRawSize());
		buffer += block.getRawSize();
	}
}

void ChunkListPacket::Deserialize(const uint8_t * buffer)
{
	buffer += sizeof(IPacket::Type);

	size_t size;
	std::memcpy(&size, buffer, sizeof(size));
	buffer += sizeof(size);

	for (size_t i = 0; i < size; i++)
	{
		ChunkPacket::ChunkData data;
		std::memcpy(&data, buffer, sizeof(data));
		buffer += sizeof(data);
		BlockRLE block;
		block.getRaw().resize(block.getRawSize());
		std::memcpy(block.getRaw().data(), buffer, block.getRawSize());
		buffer += block.getRawSize();
		m_chunks.push_back(std::make_pair(data, block));
	}
}
