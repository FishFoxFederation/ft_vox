#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class EntityMovePacket : public IServerToClientPacket
{
	EntityMovePacket();
	EntityMovePacket(uint32_t id, glm::vec3 position);
	~EntityMovePacket();

	EntityMovePacket(const EntityMovePacket& other) = delete;
	EntityMovePacket& operator=(const EntityMovePacket& other) = delete;
	
	EntityMovePacket(EntityMovePacket&& other) = delete;
	EntityMovePacket& operator=(EntityMovePacket&& other) = delete;


	void		Serialize(uint8_t * buffer) const override;
	void		Deserialize(const uint8_t * buffer) override;
	uint32_t	Size() const override;

	std::shared_ptr<IPacket> Clone() const override;

	void		Handle(const HandleArgs & args) const override;

	uint32_t	GetId() const;
	glm::vec3	GetPosition() const;

	void		SetId(uint32_t id);
	void		SetPosition(glm::vec3 position);
private:
	uint32_t	m_id;
	glm::vec3	m_position;
};
