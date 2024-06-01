#pragma once

#include "IPacket.hpp"

class DisconnectPacket : public IPacket
{
public:
	DisconnectPacket();
	DisconnectPacket(uint32_t player_id);
	~DisconnectPacket();

	DisconnectPacket(const DisconnectPacket& other);
	DisconnectPacket& operator=(const DisconnectPacket& other) = delete;

	DisconnectPacket(DisconnectPacket&& other);
	DisconnectPacket& operator=(DisconnectPacket&& other);

	virtual void			Serialize(uint8_t * buffer) const override;
	virtual void			Deserialize(const uint8_t * buffer) override;
	virtual uint32_t		Size() const override;
	virtual IPacket::Type	GetType() const override;

	virtual std::shared_ptr<IPacket> Clone() const override;

	uint32_t	GetPlayerId() const;

	void		SetPlayerId(uint32_t player_id);
private:
	uint32_t	m_player_id;
};
