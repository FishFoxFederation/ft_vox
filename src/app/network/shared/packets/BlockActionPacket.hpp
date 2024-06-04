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
	BlockActionPacket(BlockID block_id, glm::vec3 position, Action action);
	~BlockActionPacket();

	BlockActionPacket(const BlockActionPacket& other);
	BlockActionPacket& operator=(const BlockActionPacket& other);

	BlockActionPacket(BlockActionPacket&& other);
	BlockActionPacket& operator=(BlockActionPacket&& other);


	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	 * *****************************/

	BlockID		GetBlockID() const;
	glm::vec3	GetPosition() const;
	Action		GetAction() const;

	void		SetBlockID(BlockID block_id);
	void		SetPosition(glm::vec3 position);
	void		SetAction(Action action);
private:
	BlockID		m_block_id;
	glm::vec3	m_position;
	Action		m_action;
};
