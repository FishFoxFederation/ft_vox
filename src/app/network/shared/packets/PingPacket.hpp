#pragma once 


#include "IPacket.hpp"

class PingPacket : public IPacket
{
public:
	PingPacket();
	PingPacket(uint64_t id);
	PingPacket(const PingPacket&);
	PingPacket& operator=(const PingPacket&);
	PingPacket(PingPacket&&);
	PingPacket& operator=(PingPacket&&);
	~PingPacket();

	void		Serialize(uint8_t * buffer) const override;
	void		Deserialize(const uint8_t * buffer) override;
	uint32_t	Size() const override;
	enum Type	GetType() const override;
	std::shared_ptr<IPacket> Clone() const override;

	uint64_t GetId() const;
	void SetId(uint64_t id);

private:
	uint64_t m_id;
};
