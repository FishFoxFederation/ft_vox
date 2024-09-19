#include "ServerWorld.hpp"

const std::unordered_set<glm::ivec3> & ServerWorld::getBlockUpdateChunks() const
{
	return m_block_update_chunks;
}

const std::unordered_set<glm::ivec3> & ServerWorld::getEntityUpdateChunks() const
{
	return m_entity_update_chunks;
}

const ServerWorld::TicketMultiMap & ServerWorld::getTickets() const
{
	std::lock_guard lock(m_tickets_mutex);
	return m_active_tickets;
}

uint64_t ServerWorld::addTicket(const ServerWorld::Ticket & ticket)
{
	std::lock_guard lock(m_tickets_mutex);
	// LOG_INFO("Adding ticket: " << ticket.position.x << " " << ticket.position.z << " level: " << ticket.level);
	m_tickets_to_add.insert({ticket.hash(), ticket});
	return (ticket.hash());
}

void ServerWorld::removeTicket(const uint64_t & ticket_id)
{
	std::lock_guard lock(m_tickets_mutex);
	m_tickets_to_remove.insert(ticket_id);
}

uint64_t ServerWorld::changeTicket(const uint64_t & old_ticket_id, const ServerWorld::Ticket & new_ticket)
{
	std::lock_guard lock(m_tickets_mutex);
	m_tickets_to_remove.insert(old_ticket_id);
	m_tickets_to_add.insert({new_ticket.hash(), new_ticket});
	return new_ticket.hash();
}

void ServerWorld::updateTickets()
{
	ZoneScopedN("Update Tickets");
	std::lock_guard lock(m_tickets_mutex);
	bool changed = false;


	//very important to add first and remove last
	//otherwise we might not remove a ticket that was added then removed in the same tick

	if (!m_tickets_to_add.empty())
	{
		changed = true;
		for (auto & ticket : m_tickets_to_add)
			m_active_tickets.insert(ticket);
		m_tickets_to_add.clear();
		//no need to clear load levels when only adding to a floodfill
	}

	if (!m_tickets_to_remove.empty())
	{
		changed = true;
		for (auto & ticket : m_tickets_to_remove)
		{
			auto it = m_active_tickets.find(ticket);
			if (it != m_active_tickets.end())
			{
				// LOG_INFO("Removing ticket: " << ticket.position.x << " " << ticket.position.z << " level: " << ticket.level);
				m_active_tickets.erase(it);
			}
		}
		m_tickets_to_remove.clear();
		clearChunksLoadLevel();
	}

	if (changed)
	{
		floodFill(m_active_tickets);
		waitForChunkFutures();
	}
}

void ServerWorld::clearChunksLoadLevel()
{
	for (auto & [position, chunk] : m_chunks)
		chunk->setLoadLevel(TICKET_LEVEL_INACTIVE);
	m_block_update_chunks.clear();
	m_entity_update_chunks.clear();
	m_border_chunks.clear();
}

void ServerWorld::floodFill(const TicketMultiMap & tickets)
{
	// LOG_INFO("ENTERING FLOODFILL");
	std::queue<Ticket> queue;
	for (auto & [id, ticket] : tickets)
	{
		// LOG_INFO("Adding ticket to floodfill: " << ticket.position.x << " " << ticket.position.y << " " << ticket.position.z << " level: " << ticket.level);
		queue.push(ticket);
	}
	while (!queue.empty())
	{
		auto current = queue.front();
		queue.pop();
		if (current.level > WorldGenerator::MAX_TICKET_LEVEL)
			continue;

		//visit the chunk
		applyTicketToChunk(current);

		// add the neighbors
		for (int x = -1; x <= 1; x++)
		{
			for (int z = -1; z <= 1; z++)
			{
				if (x == 0 && z == 0)
					continue;
				glm::ivec3 new_position = current.position + glm::ivec3(x, 0, z);

				queue.push(Ticket{current.level + 1, new_position});
			}
		}
	}
}

void ServerWorld::applyTicketToChunk(const ServerWorld::Ticket & ticket)
{
	std::shared_ptr<Chunk> chunk = getChunk(ticket.position);
	if (chunk == nullptr)
	{
		chunk = std::make_shared<Chunk>(ticket.position);
		insertChunk(ticket.position, chunk);
	}

	chunk->status.lock_shared();
	const int highest_level = chunk->getHighestLoadLevel();
	const int current_level = chunk->getLoadLevel();
	chunk->status.unlock_shared();


	//if the chunk is currently not at a high enough generation level
	// we need to generate it
	//since generation is never downgraded we check the highest level
	if (ticket.level <= WorldGenerator::MAX_TICKET_LEVEL && 
		highest_level >= ticket.level && 
		highest_level > TICKET_LEVEL_INACTIVE) 
	{
		std::lock_guard lock(m_chunk_futures_ids_mutex);
		m_chunk_futures_ids.push_back(asyncGenChunk(ticket.position, ticket.level, current_level));
	}

	if (ticket.level <= TICKET_LEVEL_ENTITY_UPDATE)
		m_entity_update_chunks.insert(ticket.position);
	if (ticket.level <= TICKET_LEVEL_BLOCK_UPDATE)
		m_block_update_chunks.insert(ticket.position);
	if (ticket.level <= TICKET_LEVEL_BORDER)
		m_border_chunks.insert(ticket.position);

	chunk->status.lock();
	chunk->setLoadLevel(ticket.level);
	chunk->status.unlock();
}
