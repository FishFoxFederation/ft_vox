#pragma once

#include "IPacket.hpp"

class PlayerConnectedPacket : public IPacket
{
public:
	PlayerConnectedPacket();
	PlayerConnectedPacket(const uint8_t & id);
	virtual ~PlayerConnectedPacket();

	PlayerConnectedPacket(const PlayerConnectedPacket& other) = delete;
	PlayerConnectedPacket& operator=(const PlayerConnectedPacket& other) = delete;

	PlayerConnectedPacket(PlayerConnectedPacket&& other);
	PlayerConnectedPacket& operator=(PlayerConnectedPacket&& other);

	virtual void		Serialize(uint8_t * buffer) const override;
	virtual void		Deserialize(const uint8_t * buffer) override;
	virtual uint32_t	Size() const override;
	virtual IPacket::Type GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;

	uint8_t GetId() const;

	void setId(uint8_t id);
private:
	uint8_t		m_id;
};
