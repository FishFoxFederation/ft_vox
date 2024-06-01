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


	virtual void			Serialize(uint8_t * buffer) const override;
	virtual void			Deserialize(const uint8_t * buffer) override;
	virtual uint32_t		Size() const override;
	virtual IPacket::Type	GetType() const override;

	virtual std::shared_ptr<IPacket> Clone() const override;

	uint32_t	GetId() const;
	glm::vec3	GetPosition() const;

	void		SetId(uint32_t id);
	void		SetPosition(glm::vec3 position);
private:
	uint32_t	m_id;
	glm::vec3	m_position;
};
