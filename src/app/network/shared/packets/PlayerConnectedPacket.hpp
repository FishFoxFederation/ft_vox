#pragma once

#include "IPacket.hpp"

class PlayerConnectedPacket : public IPacket
{
public:
	PlayerConnectedPacket();
	PlayerConnectedPacket(const uint32_t & id);
	virtual ~PlayerConnectedPacket();

	PlayerConnectedPacket(const PlayerConnectedPacket& other) = delete;
	PlayerConnectedPacket& operator=(const PlayerConnectedPacket& other) = delete;

	PlayerConnectedPacket(PlayerConnectedPacket&& other);
	PlayerConnectedPacket& operator=(PlayerConnectedPacket&& other);

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

	void		SetPlayerId(uint32_t id);
private:
	uint32_t	m_player_id;
};
