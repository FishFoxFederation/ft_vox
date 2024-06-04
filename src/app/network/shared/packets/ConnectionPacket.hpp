#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class ConnectionPacket : public IPacket
{
public:
	ConnectionPacket();
	ConnectionPacket(uint32_t id, glm::vec3 position);
	~ConnectionPacket();

	ConnectionPacket(const ConnectionPacket& other);
	ConnectionPacket& operator=(const ConnectionPacket& other);

	ConnectionPacket(ConnectionPacket&& other);
	ConnectionPacket& operator=(ConnectionPacket&& other);


	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	 * *****************************/

	uint32_t	GetPlayerId() const;
	glm::vec3	GetPosition() const;

	void		SetPlayerId(uint32_t id);
	void		SetPosition(glm::vec3 position);
private:
	uint32_t	m_player_id;
	glm::vec3	m_position;
};
