#include "PlayerMovePacket.hpp"

PlayerMovePacket::PlayerMovePacket()
	: m_id(0), m_position(0)
{
}

PlayerMovePacket::PlayerMovePacket(uint8_t id, glm::vec3 position)
	: m_id(id), m_position(position)
{
}

PlayerMovePacket::~PlayerMovePacket()
{
}

void PlayerMovePacket::Serialize(uint8_t * buffer) const
{
	buffer[0] = m_id;
	memcpy(buffer + 1, &m_position, sizeof(glm::vec3));
}

void PlayerMovePacket::Deserialize(const uint8_t * buffer)
{
	m_id = buffer[0];
	memcpy(&m_position, buffer + 1, sizeof(glm::vec3));
}

uint32_t PlayerMovePacket::Size() const
{
	return sizeof(uint8_t) + sizeof(glm::vec3);
}

std::shared_ptr<IPacket> PlayerMovePacket::Clone() const
{
	return std::make_shared<PlayerMovePacket>();
}

void PlayerMovePacket::Handle(const HandleArgs & args) const
{
	if (args.env == HandleArgs::Env::CLIENT)
		args.world->updatePlayerPosition(m_id, m_position);
	else if (args.env == HandleArgs::Env::SERVER)
	{
		auto packet = std::make_shared<PlayerMovePacket>(m_id, m_position);
		packet->SetConnectionId(GetConnectionId());
		args.server->sendAll(packet);		
	}
}
