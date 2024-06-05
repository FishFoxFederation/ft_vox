#pragma once

#include "IPacket.hpp"
#include "glm/glm.hpp"

class ChunkRequestPacket : public IPacket
{
public:
	ChunkRequestPacket();
	~ChunkRequestPacket();

	ChunkRequestPacket(ChunkRequestPacket & other);
	ChunkRequestPacket & operator=(ChunkRequestPacket & other);
	ChunkRequestPacket(ChunkRequestPacket && other);
	ChunkRequestPacket & operator=(ChunkRequestPacket && other);
	
	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	********************************/
	glm::ivec3		GetChunkPos() const;

	void			SetChunkPos(const glm::ivec3 & chunk_pos);
private:
	glm::ivec3			m_chunk_pos;
};
