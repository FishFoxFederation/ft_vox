#pragma once

#include "IPacket.hpp"
#include "Chunk.hpp"
#include "RLE_TEST.hpp"
#include "ChunkPacket.hpp"


class ChunkListPacket : public IPacket
{
	typedef ChunkPacket::BlockRLE BlockRLE;
public:
	constexpr static IPacket::Type getType()
	{
		return IPacket::Type::CHUNK_LIST;
	};
	ChunkListPacket();
	ChunkListPacket(const std::vector<std::shared_ptr<Chunk>> & chunks);
	~ChunkListPacket();

	ChunkListPacket(const ChunkListPacket& other);
	ChunkListPacket& operator=(const ChunkListPacket& other);
	ChunkListPacket(ChunkListPacket&& other);
	ChunkListPacket& operator=(ChunkListPacket&& other);

	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	 * *******************************/

	std::vector<std::shared_ptr<Chunk>> GetChunks() const;

	void	SetChunks(const std::vector<std::shared_ptr<Chunk>> & chunks);

private:
	std::vector<std::pair<ChunkPacket::ChunkData, ChunkPacket::BlockRLE>> m_chunks;
};
