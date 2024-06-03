#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class PlayerMovePacket : public IPacket
{
public:
	PlayerMovePacket();
	PlayerMovePacket(uint8_t id, glm::dvec3 position, glm::dvec3 displacement);
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
	glm::dvec3 GetPosition() const;
	glm::dvec3 GetDisplacement() const;

	void SetId(uint8_t id);
	void SetPosition(glm::dvec3 position);
	void SetDisplacement(glm::dvec3 displacement);
private:
	uint8_t		m_id;
	glm::dvec3	m_position;
	glm::dvec3	m_displacement;
};
