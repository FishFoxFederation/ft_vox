#pragma once

#include <mutex>
#include <queue>

#include "Tracy.hpp"
#include "Chunk.hpp"
#include "define.hpp"
#include "ServerWorld.hpp"


// does a chunk know about its load level ?


class TicketManager
{
public:
	// lower value means higher load level
	constexpr static int TICKET_LEVEL_ENTITY_UPDATE = 31;
	constexpr static int TICKET_LEVEL_BLOCK_UPDATE = 32;
	constexpr static int TICKET_LEVEL_BORDER = 33;
	constexpr static int TICKET_LEVEL_INACTIVE = 34;
	struct Ticket
	{
		int level;
		glm::ivec3 position;
	};
	TicketManager(
		ServerWorld & world
	);
	~TicketManager();

	TicketManager(TicketManager & other) = delete;
	TicketManager(TicketManager && other) = delete;
	TicketManager & operator=(TicketManager & other) = delete;
	TicketManager & operator=(TicketManager && other) = delete;

	std::vector<glm::ivec3> getBlockUpdateChunks() const;
	std::vector<glm::ivec3> getEntityUpdateChunks() const;
	std::vector<Ticket>		getTickets() const;


	void					addTicket(const Ticket & ticket);
	void 					removeTicket(const Ticket & ticket);

	void					updateTickets();
private:
	std::vector<Ticket> m_active_tickets;
	std::vector<Ticket> m_tickets_to_add;
	std::vector<Ticket> m_tickets_to_remove;

	ServerWorld & m_world;


	void floodFill(const std::vector<TicketManager::Ticket> & tickets);
	void clearChunksLevels();
};
