#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class PlayerMovePacket : public IPacket
{
public:
	PlayerMovePacket();
	PlayerMovePacket(uint64_t id, glm::dvec3 position, glm::dvec3 displacement);
	~PlayerMovePacket();

	PlayerMovePacket(const PlayerMovePacket& other);
	PlayerMovePacket& operator=(const PlayerMovePacket& other);

	PlayerMovePacket(PlayerMovePacket&& other);
	PlayerMovePacket& operator=(PlayerMovePacket&& other);

	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	**ATTRIBUTES
	*******************************/

	uint64_t	GetPlayerId() const;
	glm::dvec3	GetPosition() const;
	glm::dvec3	GetDisplacement() const;

	void		SetPlayerId(uint64_t id);
	void		SetPosition(glm::dvec3 position);
	void		SetDisplacement(glm::dvec3 displacement);
private:
	uint64_t	m_player_id;
	glm::dvec3	m_position;
	glm::dvec3	m_displacement;
};
