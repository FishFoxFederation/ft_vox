#pragma once

#include "../IPacket.hpp"
#include "glm/vec3.hpp"

class ConnectionPacket : public IPacket
{
public:
	ConnectionPacket();
	ConnectionPacket(uint8_t id, glm::vec3 position);
	~ConnectionPacket();

	ConnectionPacket(const ConnectionPacket& other) = delete;
	ConnectionPacket& operator=(const ConnectionPacket& other) = delete;

	ConnectionPacket(ConnectionPacket&& other) = delete;
	ConnectionPacket& operator=(ConnectionPacket&& other) = delete;


	void		Serialize(uint8_t * buffer) const override;
	void		Deserialize(const uint8_t * buffer) override;
	uint32_t	Size() const override;

	std::shared_ptr<IPacket> Clone() const override;

	void		Handle(World& world) const override;

	uint8_t		GetId() const;
	glm::vec3	GetPosition() const;

	void		SetId(uint8_t id);
	void		SetPosition(glm::vec3 position);
private:
	uint8_t		m_id;
	glm::vec3	m_position;
};
