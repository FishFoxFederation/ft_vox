#pragma once

#include "IPacket.hpp"
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

	void		Handle(const HandleArgs & args) const override;
	

	uint8_t GetId() const;

	void setId(uint8_t id);
private:
	uint8_t		m_id;
};
