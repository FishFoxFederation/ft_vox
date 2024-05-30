#pragma once

#include "IPacket.hpp"

class PlayerMovePacket : public IPacket
{
public:
	PlayerMovePacket();
	PlayerMovePacket(uint8_t id, glm::vec3 position);
	~PlayerMovePacket();

	PlayerMovePacket(const PlayerMovePacket& other) = delete;
	PlayerMovePacket& operator=(const PlayerMovePacket& other) = delete;

	PlayerMovePacket(PlayerMovePacket&& other) = delete;
	PlayerMovePacket& operator=(PlayerMovePacket&& other) = delete;

	void Serialize(uint8_t * buffer) const override;
	void Deserialize(const uint8_t * buffer) override;
	uint32_t Size() const override;

	std::shared_ptr<IPacket> Clone() const override;

	void Handle(const HandleArgs & args) const override;

	uint8_t GetId() const;
	glm::vec3 GetPosition() const;

	void SetId(uint8_t id);
	void SetPosition(glm::vec3 position);
private:
	uint8_t		m_id;
	glm::vec3	m_position;
};
