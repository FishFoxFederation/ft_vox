#pragma once

#include "IPacket.hpp"
#include "glm/vec3.hpp"

class PlayerListPacket : public IPacket
{
public:
	struct PlayerInfo
	{
		uint64_t id;
		glm::dvec3 position;
	};
	PlayerListPacket();
	PlayerListPacket(const std::vector<PlayerInfo> & players);
	~PlayerListPacket();

	PlayerListPacket(const PlayerListPacket& other);
	PlayerListPacket& operator=(const PlayerListPacket& other);
	PlayerListPacket(PlayerListPacket&& other);
	PlayerListPacket& operator=(PlayerListPacket&& other);

	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	/*******************************
	 * ATTRIBUTES
	 * *******************************/

	const std::vector<PlayerInfo> & GetPlayers() const;

	void	SetPlayers(const std::vector<PlayerInfo> & players);
private:
	std::vector<PlayerInfo> m_players;
};
