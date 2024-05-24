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

	void		Handle(const HandleArgs & args) const override
	{
		//code executed inside the server
		if (args.env == HandleArgs::Env::SERVER)
		{
			static uint8_t id = 0;
			std::shared_ptr<ConnectionPacket> packet = std::make_shared<ConnectionPacket>(id++, glm::vec3(0, 255, 0));
			packet->SetConnectionId(GetConnectionId());

			args.server->send(packet);
		}
	}
};
