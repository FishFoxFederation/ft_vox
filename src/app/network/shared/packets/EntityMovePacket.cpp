#include "EntityMovePacket.hpp"

EntityMovePacket::EntityMovePacket()
	: m_id(0), m_position(0)
{
}

EntityMovePacket::EntityMovePacket(uint32_t id, glm::vec3 position)
	: m_id(id), m_position(position)
{
}

EntityMovePacket::~EntityMovePacket()
{
}

void EntityMovePacket::Serialize(uint8_t * buffer) const
{
	memcpy(buffer, &m_id, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &m_position, sizeof(glm::vec3));
}

void EntityMovePacket::Deserialize(const uint8_t * buffer)
{
	memcpy(&m_id, buffer, sizeof(uint32_t));
	memcpy(&m_position, buffer + sizeof(uint32_t), sizeof(glm::vec3));
}

uint32_t EntityMovePacket::Size() const
{
	return sizeof(uint32_t) + sizeof(glm::vec3);
}

std::shared_ptr<IPacket> EntityMovePacket::Clone() const
{
	return std::make_shared<EntityMovePacket>();
}

void EntityMovePacket::Handle(const HandleArgs & args) const
{
}
