#pragma once

#include "IPacket.hpp"

#include <queue>
#include <memory>
#include <mutex>
#include "Tracy.hpp"

class IncomingPacketList
{
public:
	IncomingPacketList();
	~IncomingPacketList();

	IncomingPacketList(const IncomingPacketList& other) = delete;
	IncomingPacketList& operator=(const IncomingPacketList& other) = delete;

	IncomingPacketList(IncomingPacketList&& other);
	IncomingPacketList& operator=(IncomingPacketList&& other);

	void						push(std::shared_ptr<IPacket> packet);
	std::shared_ptr<IPacket>	pop();
	size_t						size() const;
	bool						empty() const;
private:
	std::queue<std::shared_ptr<IPacket>>	m_packets;
	mutable TracyLockableN(std::mutex, m_mutex, "Incoming Packet List Mutex");
};
