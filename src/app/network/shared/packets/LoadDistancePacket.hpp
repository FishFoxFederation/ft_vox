#pragma once

#include "IPacket.hpp"


class LoadDistancePacket : public IPacket
{
public:
	LoadDistancePacket();
	LoadDistancePacket(uint32_t distance);
	~LoadDistancePacket();

	LoadDistancePacket(const LoadDistancePacket& other);
	LoadDistancePacket& operator=(const LoadDistancePacket& other);

	LoadDistancePacket(LoadDistancePacket&& other);
	LoadDistancePacket& operator=(LoadDistancePacket&& other);

	void			Serialize(uint8_t * buffer) const override;
	void			Deserialize(const uint8_t * buffer) override;
	uint32_t		Size() const override;
	bool			HasDynamicSize() const override;
	IPacket::Type	GetType() const override;

	std::shared_ptr<IPacket> Clone() const override;
	/*********************************\ 
	 * ATTRIBUTES
	\*********************************/

	uint32_t	GetDistance() const;

	void		SetDistance(uint32_t distance);
private:
	uint32_t m_distance;
};
