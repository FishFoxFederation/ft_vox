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
	LOG_INFO("Adding ticket: " << ticket.position.x << " " << ticket.position.z << " level: " << ticket.level);
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
	std::lock_guard lock(m_chunks_mutex);
	std::lock_guard lock2(m_tickets_mutex);
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
		ChunkGenList chunk_gen_list;

		floodFill(m_active_tickets, chunk_gen_list);
		doChunkGens(chunk_gen_list);
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

void ServerWorld::floodFill(const TicketMultiMap & tickets, ChunkGenList & chunk_gen_list)
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
		if (!applyTicketToChunk(current, chunk_gen_list))
			continue;

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

bool ServerWorld::applyTicketToChunk(const ServerWorld::Ticket & ticket, ServerWorld::ChunkGenList & chunk_gen_list)
{
	std::shared_ptr<Chunk> chunk = getChunkNoLock(ticket.position);
	if (chunk == nullptr)
	{
		chunk = std::make_shared<Chunk>(ticket.position);
		insertChunkNoLock(ticket.position, chunk);
	}
	std::lock_guard lock(chunk->status);

	const int current_ticket_level = chunk->getLoadLevel();
	const Chunk::genLevel current_gen_level = chunk->getGenLevel();

	if (current_ticket_level < ticket.level || ticket.level >= WorldGenerator::MAX_TICKET_LEVEL)
		return false;

	//if the chunk is currently not at a high enough generation level
	// we need to generate it
	//since generation is never downgraded we check the highest level
	Chunk::genLevel desired_gen_level = WorldGenerator::ticketToGenLevel(ticket.level);
		
	
	if (
		desired_gen_level != Chunk::genLevel::EMPTY &&
		desired_gen_level < current_gen_level)
	{
		//add chunk to the list of chunks that need to be generated associated with its level
		chunk_gen_list.insert({ticket.position, desired_gen_level});

		// std::lock_guard lock(m_chunk_futures_ids_mutex);
		// m_chunk_futures_ids.push_back(asyncGenChunk(ticket.position, desired_gen_level, current_gen_level));
	}

	if (ticket.level <= TICKET_LEVEL_ENTITY_UPDATE)
		m_entity_update_chunks.insert(ticket.position);
	if (ticket.level <= TICKET_LEVEL_BLOCK_UPDATE)
		m_block_update_chunks.insert(ticket.position);
	if (ticket.level <= TICKET_LEVEL_BORDER)
		m_border_chunks.insert(ticket.position);

	chunk->setLoadLevel(ticket.level);
	return true;
}
