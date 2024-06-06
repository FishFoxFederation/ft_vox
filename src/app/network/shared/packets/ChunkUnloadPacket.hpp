#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class ChunkUnloadPacket : public IPacket
{
public:
	ChunkUnloadPacket();
	ChunkUnloadPacket(const glm::ivec3 & chunk_position);
	~ChunkUnloadPacket();

	ChunkUnloadPacket(const ChunkUnloadPacket & other);
	ChunkUnloadPacket & operator=(const ChunkUnloadPacket & other);
	ChunkUnloadPacket(ChunkUnloadPacket && other);
	ChunkUnloadPacket & operator=(ChunkUnloadPacket && other);

	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	 * ****************************/
	glm::ivec3	GetChunkPosition() const;

	void		SetChunkPosition(const glm::ivec3 & chunk_position);
private:
	glm::ivec3 m_chunk_pos;
};
