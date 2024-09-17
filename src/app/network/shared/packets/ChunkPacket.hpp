#pragma once

#include "IPacket.hpp"
#include "Chunk.hpp"


class ChunkPacket : public IPacket
{
public:
	struct ChunkData
	{
		glm::ivec3			chunk_pos;
		Chunk::BlockArray	blocks;
		Chunk::LightArray	light;
	};

	ChunkPacket();
	ChunkPacket(const Chunk & chunk);
	~ChunkPacket();

	ChunkPacket(ChunkPacket & other);
	ChunkPacket & operator=(ChunkPacket & other);
	ChunkPacket(ChunkPacket && other);
	ChunkPacket & operator=(ChunkPacket && other);
	
	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	********************************/
	std::shared_ptr<Chunk>			GetChunk() const;

	void							SetChunk(const Chunk & chunk);

private:
	ChunkData	m_chunk_data;
};

