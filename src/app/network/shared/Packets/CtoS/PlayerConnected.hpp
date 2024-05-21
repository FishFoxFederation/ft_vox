#pragma once

#include "../IPacket.hpp"
#include "ConnectionPacket.hpp"

class PlayerConnectedPacket : public IPacket
{
public:
	PlayerConnectedPacket();
	virtual ~PlayerConnectedPacket();

	PlayerConnectedPacket(const PlayerConnectedPacket& other) = delete;
	PlayerConnectedPacket& operator=(const PlayerConnectedPacket& other) = delete;

	PlayerConnectedPacket(PlayerConnectedPacket&& other);
	PlayerConnectedPacket& operator=(PlayerConnectedPacket&& other);

	void		Serialize(uint8_t * buffer) const override;
	void		Deserialize(const uint8_t * buffer) override;
	uint32_t	Size() const override;

	std::shared_ptr<IPacket> Clone() const override;

	void		Handle(Server & server) const override
	{
		
	}

	uint64_t	GetConnectionId() const;
};
