#pragma once

#include "IPacket.hpp"
#include "Block.hpp"

class BlockActionPacket : public IPacket
{
public:
	enum class Action : uint8_t
	{
		PLACE
	};

	BlockActionPacket();
	BlockActionPacket(BlockID block_id, glm::ivec3 position, Action action);
	~BlockActionPacket();

	BlockActionPacket(const BlockActionPacket& other);
	BlockActionPacket& operator=(const BlockActionPacket& other);

	BlockActionPacket(BlockActionPacket&& other);
	BlockActionPacket& operator=(BlockActionPacket&& other);


	virtual void			Serialize(uint8_t * buffer) const override;
	virtual void			Deserialize(const uint8_t * buffer) override;
	virtual uint32_t		Size() const override;
	virtual IPacket::Type	GetType() const override;

	virtual std::shared_ptr<IPacket> Clone() const override;

	BlockID	GetBlockID() const;
	glm::ivec3	GetPosition() const;
	Action	GetAction() const;

	void	SetBlockID(BlockID block_id);
	void	SetPosition(glm::ivec3 position);
	void	SetAction(Action action);
private:
	BlockID		m_block_id;
	glm::ivec3	m_position;
	Action		m_action;
};
