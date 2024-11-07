#pragma once 


#include "IPacket.hpp"

class PingPacket : public IPacket
{
public:
	PingPacket();
	PingPacket(uint64_t id, uint8_t counter);
	PingPacket(const PingPacket&);
	PingPacket& operator=(const PingPacket&);
	PingPacket(PingPacket&&);
	PingPacket& operator=(PingPacket&&);
	~PingPacket();

	void		Serialize(uint8_t * buffer) const override;
	void		Deserialize(const uint8_t * buffer) override;
	uint32_t	Size() const override;
	bool		HasDynamicSize() const override;
	enum Type	GetType() const override;
	std::shared_ptr<IPacket> Clone() const override;

	uint64_t GetId() const;
	void SetId(uint64_t id);

	uint8_t GetCounter() const;
	void SetCounter(uint8_t counter);
private:
	uint64_t m_id;
	uint8_t  m_counter;
};
