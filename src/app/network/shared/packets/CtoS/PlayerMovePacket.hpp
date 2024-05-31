#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class PlayerMovePacket : public IPacket
{
public:
	PlayerMovePacket();
	PlayerMovePacket(uint8_t id, glm::vec3 position, glm::vec3 displacement);
	~PlayerMovePacket();

	PlayerMovePacket(const PlayerMovePacket& other);
	PlayerMovePacket& operator=(const PlayerMovePacket& other) = delete;

	PlayerMovePacket(PlayerMovePacket&& other);
	PlayerMovePacket& operator=(PlayerMovePacket&& other);

	virtual void Serialize(uint8_t * buffer) const override;
	virtual void Deserialize(const uint8_t * buffer) override;
	virtual uint32_t Size() const override;
	virtual IPacket::Type GetType() const override;

	virtual std::shared_ptr<IPacket> Clone() const override;

	uint8_t GetId() const;
	glm::vec3 GetPosition() const;
	glm::vec3 GetDisplacement() const;

	void SetId(uint8_t id);
	void SetPosition(glm::vec3 position);
	void SetDisplacement(glm::vec3 displacement);
private:
	uint8_t		m_id;
	glm::vec3	m_position;
	glm::vec3	m_displacement;
};
